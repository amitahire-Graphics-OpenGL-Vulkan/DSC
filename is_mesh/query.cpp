//
// Created by Morten Nobel-Jørgensen on 19/11/14.
// Copyright (c) 2014 ___FULLUSERNAME___. All rights reserved.
//

#include "Query.h"
#include "is_mesh/is_mesh.h"
#include <limits>

using namespace CGLA;

/**
* Pseudocode
* 1. Find "back" boundary triangle (O(n))
* 2. Find enclosing tetrahedron
* 3. While tetrahedron exists
* 3.1 Find intersection with other faces on tetrahedron
* 3.2 Find new tetrahedron
*/
namespace is_mesh {

    Query::Query(ISMesh *mesh)
            : mesh(mesh) {

    }

    Query::~Query() {
        delete boundary_faces;
    }

    QueryResult Query::raycast_faces(Ray ray, QueryType queryType) {
        if (!boundary_faces){
            rebuild_boundary_cache();
        }
        FaceKey firstIntersection((unsigned int) -1);
        double dist = std::numeric_limits<double>::max();

        for (auto faceIter : *boundary_faces){
            Face & face = mesh->get(faceIter);
            auto nodePos = mesh->get_pos(face.node_keys());
            double newDist;
            if (ray.intersect_triangle(nodePos[0], nodePos[1], nodePos[2], newDist)){
                if (dist != std::numeric_limits<double>::max()){
                    if (newDist  < dist){
                        firstIntersection = faceIter;
                    }
                    break;
                }
                firstIntersection = faceIter;
                dist = newDist;
            }
        }
        if ((unsigned int)firstIntersection == (unsigned int)-1){
            return QueryResult();
        }

        return QueryResult(firstIntersection, dist, ray, queryType, mesh);
    }

    void Query::rebuild_boundary_cache() {
        if (!boundary_faces){
            boundary_faces = new std::vector<FaceKey>();
        } else {
            boundary_faces->clear();
        }

        for (auto faceiter : mesh->faces()){
            if (faceiter->is_boundary()){
                boundary_faces->push_back(faceiter.key());
            }
        }
    }

    QueryResultIterator::QueryResultIterator() {
        face_key = FaceKey((unsigned int) -1);
    }

    QueryResultIterator::QueryResultIterator(FaceKey const &first_intersection, double dist, Ray const &ray, QueryType const &query_type, ISMesh *mesh)
            : first_intersection(first_intersection), dist(dist), ray(ray), query_type(query_type), mesh(mesh) {
        FaceKey lastFace = first_intersection;
        tetrahedron_key = mesh->get(first_intersection).tet_keys(); // since boundary always only one
        std::vector<FaceKey> res;
        if (dist >= 0 && query_type != QueryType::Interface){
            face_key = lastFace;
        } else {
            next();
        }
    }

    void QueryResultIterator::next() {
        if (tetrahedron_key.size()==0){
            face_key = FaceKey((unsigned int) -1);
        }
        while (tetrahedron_key.size()>0){
            SimplexSet<FaceKey> faces = mesh->get(tetrahedron_key.front()).face_keys() - face_key;
            double newDist = std::numeric_limits<double>::max();
            for (FaceKey current_face_key : faces) {
                auto & face = mesh->get(current_face_key);
                auto nodePos = mesh->get_pos(face.node_keys());
                if (ray.intersect_triangle(nodePos[0], nodePos[1], nodePos[2], newDist)) {
                    face_key = current_face_key;
                    tetrahedron_key = face.get_co_boundary() - tetrahedron_key;
                    if (newDist >=0 && (query_type == QueryType::All ||
                            (query_type == QueryType::Interface && face.is_interface()) ||
                            (query_type == QueryType::Boundary && face.is_boundary()))){
                        return;
                    }
                    break;
                }
            }
        }
        face_key = FaceKey((unsigned int) -1);
    }

    FaceKey QueryResultIterator::operator*() { return face_key; }

    bool QueryResultIterator::operator!=(const QueryResultIterator & other) {
        return !(face_key == other.face_key);
    }

    QueryResult::QueryResult(FaceKey const &first_intersection, double dist, Ray const &ray, QueryType const &query_type, ISMesh *mesh)
            : first_intersection(first_intersection), dist(dist), ray(ray), query_type(query_type), mesh(mesh) {
    }

    QueryResultIterator QueryResult::begin() {
        if (mesh == nullptr){
            return end();
        }
        return QueryResultIterator{first_intersection, dist, ray, query_type, mesh};
    }

    QueryResultIterator QueryResult::end() {
        return QueryResultIterator{};
    }

    QueryResultIterator &QueryResultIterator::operator++() {
        next();
        return *this;
    }

    QueryResultIterator::QueryResultIterator(const QueryResultIterator &other)
        :first_intersection(other.first_intersection),
        dist(other.dist),
        ray(other.ray),
        query_type(other.query_type),
        tetrahedron_key(other.tetrahedron_key),
        face_key(other.face_key),
        mesh(other.mesh)
    {
    }


    std::vector<NodeKey> Query::neighborhood(NodeKey from_node, double max_distance) {
        // naive implementation - todo implement graph centric version
        vec3 from_pos = mesh->get(from_node).get_pos();
        std::vector<NodeKey> res;
        double max_distanceSqr = max_distance * max_distance;
        for (auto nodeiter : mesh->nodes()){
            vec3 to_pos = nodeiter->get_pos();
            double lengthSqr = sqr_length(from_pos - to_pos);
            if (lengthSqr < max_distanceSqr){
                res.push_back(nodeiter.key());
            }
        }
        return res;
    }
}