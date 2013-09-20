//
//  is_mesh_API.h
//  DSC
//
//  Created by Asger Nyman Christiansen on 6/14/13.
//  Copyright (c) 2013 Asger Nyman Christiansen. All rights reserved.
//

#pragma once

#include <is_mesh/is_mesh.h>
#include <is_mesh/io/is_mesh_lists_read.h>


template <typename node_traits, typename edge_traits, typename face_traits, typename tet_traits>
class ISMesh
{
    typedef typename OpenTissue::is_mesh::t4mesh< node_traits, tet_traits, edge_traits, face_traits> Mesh;
        
public:
    typedef typename Mesh::node_key_type            node_key;
    typedef typename Mesh::edge_key_type            edge_key;
    typedef typename Mesh::face_key_type            face_key;
    typedef typename Mesh::tetrahedron_key_type     tet_key;
    typedef typename Mesh::simplex_set_type         simplex_set;
    
    
    const node_key NULL_NODE;
    const edge_key NULL_EDGE;
    const face_key NULL_FACE;
    const tet_key NULL_TETRAHEDRON;
    
private:
    Mesh mesh;
    
public:
    template<typename real>
    ISMesh(std::vector<real> & points, std::vector<int> & tets): NULL_NODE(-1), NULL_EDGE(-1), NULL_FACE(-1), NULL_TETRAHEDRON(-1)
    {
        vectors_read(points, tets, mesh);
        init();
        check_validity();
    }
    
    ///////////////
    // ITERATORS //
    ///////////////
public:
    typename Mesh::node_iterator nodes_begin()
    {
        return mesh.nodes_begin();
    }
    
    typename Mesh::node_iterator nodes_end()
    {
        return mesh.nodes_end();
    }
    
    typename Mesh::edge_iterator edges_begin()
    {
        return mesh.edges_begin();
    }
    
    typename Mesh::edge_iterator edges_end()
    {
        return mesh.edges_end();
    }
    
    typename Mesh::face_iterator faces_begin()
    {
        return mesh.faces_begin();
    }
    
    typename Mesh::face_iterator faces_end()
    {
        return mesh.faces_end();
    }
    
    typename Mesh::tetrahedron_iterator tetrahedra_begin()
    {
        return mesh.tetrahedra_begin();
    }
    
    typename Mesh::tetrahedron_iterator tetrahedra_end()
    {
        return mesh.tetrahedra_end();
    }
    
    /////////////////////
    // LABEL FUNCTIONS //
    /////////////////////
public:
    template<typename key>
    bool is_interface(const key& k)
    {
        return get(k).is_interface();
    }
    
    template<typename key>
    bool is_boundary(const key& k)
    {
        return get(k).is_boundary();
    }
    
    template<typename key>
    bool is_crossing(const key& k)
    {
        return get(k).is_crossing();
    }
    
    int get_label(const tet_key& t)
    {
        return get(t).label();
    }
    
private:
    template<typename key>
    void set_interface(const key& k, bool b)
    {
        return get(k).set_interface(b);
    }
    
    template<typename key>
    void set_boundary(const key& k, bool b)
    {
        return get(k).set_boundary(b);
    }
    
    template<typename key>
    void set_crossing(const key& k, bool b)
    {
        return get(k).set_crossing(b);
    }
    
public:
    void set_label(const tet_key& t, int label)
    {
        get(t).label(label);
        simplex_set cl_t;
        closure(t, cl_t);
        update(cl_t);
    }
    
private:
    /**
     * Perform an initial update of flags for all nodes, edges and faces.
     */
    void init()
    {        
        for (auto fit = faces_begin(); fit != faces_end(); fit++)
        {
            update_flag(fit.key());
        }
        
        for (auto eit = edges_begin(); eit != edges_end(); eit++)
        {
            update_flag(eit.key());
        }
        
        for (auto nit = nodes_begin(); nit != nodes_end(); nit++)
        {
            update_flag(nit.key());
        }
    }
    
    /**
     * Updates the flags (is interface, is boundary, is crossing) of simplices in set.
     */
    void update(simplex_set & set)
    {
        // Update faces
        for (auto fit = set.faces_begin(); fit != set.faces_end(); fit++)
        {
            if (exists(*fit))
            {
                update_flag(*fit);
            }
        }
        
        // Update edges
        for (auto eit = set.edges_begin(); eit != set.edges_end(); eit++)
        {
            if (exists(*eit))
            {
                update_flag(*eit);
            }
        }
        
        // Update nodes
        for (auto nit = set.nodes_begin(); nit != set.nodes_end(); nit++)
        {
            if (exists(*nit))
            {
                update_flag(*nit);
            }
        }
    }
    
    void update_flag(const face_key & f)
    {
        set_interface(f, false);
        set_boundary(f, false);
        
        simplex_set st_f;
        star(f, st_f);
        if (st_f.size_tetrahedra() == 1)
        {
            // On the boundary
            set_boundary(f, true);
            if (get_label(*(st_f.tetrahedra_begin())) != 0)
            {
                set_interface(f, true);
            }
        }
        else if(st_f.size_tetrahedra() == 2)
        {
            auto tit = st_f.tetrahedra_begin();
            int label0 = get_label(*tit);   ++tit;
            int label1 = get_label(*tit);
            if (label0 != label1)
            {
                // On the interface
                set_interface(f, true);
            }
        }
    }
    
    void update_flag(const edge_key & e)
    {
        set_boundary(e, false);
        set_interface(e, false);
        set_crossing(e, false);
        
        simplex_set st_e;
        star(e, st_e);
        
        int i = 0;
        for (auto fit = st_e.faces_begin(); fit != st_e.faces_end(); fit++)
        {
            if (exists(*fit))
            {
                if (is_boundary(*fit))
                {
                    set_boundary(e, true);
                }
                if (is_interface(*fit))
                {
                    set_interface(e, true);
                    i++;
                }
            }
        }
        if(i > 2)
        {
            set_crossing(e, true);
        }
    }
    
    void connected_component(simplex_set& st_n, const tet_key& t)
    {
        int label = get_label(t);
        st_n.erase(t);
        simplex_set cl_t;
        closure(t, cl_t);
        
        for(auto fit = cl_t.faces_begin(); fit != cl_t.faces_end(); fit++)
        {
            tet_key t2 = get_tet(t, *fit);
            if(st_n.contains(t2) && label == get_label(t2))
            {
                connected_component(st_n, t2);
            }
        }
    }
public:
    bool crossing(const node_key& n)
    {
        simplex_set st_n;
        star(n, st_n);
        
        int c = 0;
        while (st_n.size_tetrahedra() > 0)
        {
            if(c == 2)
            {
                return true;
            }
            tet_key t = *st_n.tetrahedra_begin();
            connected_component(st_n, t);
            c++;
        }
        return false;
    }
    
    void update_flag(const node_key & n)
    {
        set_interface(n, false);
        set_boundary(n, false);
        set_crossing(n, false);
        
        simplex_set st_n;
        star(n, st_n);
        for (auto eit = st_n.edges_begin(); eit != st_n.edges_end(); eit++)
        {
            if (exists(*eit))
            {
                if (is_interface(*eit))
                {
                    set_interface(n, true);
                }
                if (is_boundary(*eit))
                {
                    set_boundary(n, true);
                }
                if (is_crossing(*eit))
                {
                    set_crossing(n, true);
                }
            }
        }
        if(!is_crossing(n) && is_interface(n) && crossing(n))
        {
            set_crossing(n, true);
        }
    }
    
    //////////////////////
    // GETTER FUNCTIONS //
    //////////////////////
public:
    typename Mesh::node_type & get(const node_key& k)
    {
        return mesh.find_node(k);
    }
    
    typename Mesh::edge_type & get(const edge_key& k)
    {
        return mesh.find_edge(k);
    }
    
    typename Mesh::face_type & get(const face_key& k)
    {
        return mesh.find_face(k);
    }
    
    typename Mesh::tetrahedron_type & get(const tet_key& k)
    {
        return mesh.find_tetrahedron(k);
    }
    

    std::vector<node_key> get_nodes(const edge_key& e)
    {
        std::vector<node_key> nodes(2);
        mesh.vertices(e, nodes);
        return nodes;
    }
    
    std::vector<node_key> get_nodes(const face_key& f)
    {
        orient_face(f);
        std::vector<node_key> nodes(3);
        mesh.vertices(f, nodes);
        return nodes;
    }
    
    std::vector<node_key> get_nodes(const tet_key& tid)
    {
        std::vector<node_key> nodes(4);
        mesh.vertices(tid, nodes);
        return nodes;
    }
    
    std::vector<edge_key> get_edges(const face_key& fid)
    {
        std::vector<edge_key> edges;
        simplex_set cl_f;
        closure(fid, cl_f);
        for(auto eit = cl_f.edges_begin(); eit != cl_f.edges_end(); eit++)
        {
            edges.push_back(*eit);
        }
        return edges;
    }
    
    std::vector<edge_key> get_edges(const tet_key& tid)
    {
        std::vector<edge_key> edges;
        simplex_set cl_t;
        closure(tid, cl_t);
        for(auto eit = cl_t.edges_begin(); eit != cl_t.edges_end(); eit++)
        {
            edges.push_back(*eit);
        }
        return edges;
    }
    
    edge_key get_edge(const node_key& n1, const node_key& n2)
    {
        simplex_set st1, st2;
        star(n1, st1);
        star(n2, st2);
        st1.intersection(st2);
        
        if (st1.size_edges() != 1)
        {
            return NULL_EDGE;
        }
        return *(st1.edges_begin());
    }
    
    edge_key get_edge(const face_key& f1, const face_key& f2)
    {
        simplex_set cl1, cl2;
        closure(f1, cl1);
        closure(f2, cl2);
        cl1.intersection(cl2);
        
        if (cl1.size_edges() != 1)
        {
            return NULL_EDGE;
        }
        return *(cl1.edges_begin());
    }
    
    edge_key get_edge(const tet_key& t1, const tet_key& t2, const tet_key& t3)
    {
        simplex_set cl1, cl2, cl3;
        closure(t1, cl1);
        closure(t2, cl2);
        closure(t3, cl3);
        cl1.intersection(cl2);
        cl1.intersection(cl3);
        
        if (cl1.size_edges() != 1)
        {
            return NULL_EDGE;
        }
        return *(cl1.edges_begin());
    }
    
    face_key get_face(const node_key& n1, const node_key& n2, const node_key& n3)
    {
        simplex_set st1, st2, st3;
        star(n1, st1);
        star(n2, st2);
        star(n3, st3);
        
        st1.intersection(st2);
        st1.intersection(st3);
        
        if (st1.size_faces() != 1)
        {
            return NULL_FACE;
        }
        return *(st1.faces_begin());
    }
    
    face_key get_face(const tet_key& t1, const tet_key& t2)
    {
        simplex_set cl1, cl2;
        closure(t1, cl1);
        closure(t2, cl2);
        cl1.intersection(cl2);
        
        if (cl1.size_faces() != 1)
        {
            return NULL_FACE;
        }
        return *(cl1.faces_begin());
    }
    
    tet_key get_tet(const tet_key& t, const face_key& f)
    {
        simplex_set st_f;
        star(f, st_f);
        for(auto tit = st_f.tetrahedra_begin(); tit != st_f.tetrahedra_end(); tit++)
        {
            if(*tit != t)
            {
                return *tit;
            }
        }
        return NULL_TETRAHEDRON;
    }
    
    node_key get_apex(const tet_key& t, const face_key& f)
    {
        simplex_set cl_f, cl_t;
        closure(t, cl_t);
        closure(f, cl_f);
        cl_t.difference(cl_f);
        return *cl_t.nodes_begin();
    }
    
    node_key get_apex(const face_key& f, const edge_key& e)
    {
        simplex_set cl_f, cl_e;
        closure(f, cl_f);
        closure(e, cl_e);
        cl_f.difference(cl_e);
#ifdef DEBUG
        assert(cl_f.size_nodes() == 1);
#endif
        return *cl_f.nodes_begin();
    }
    
    std::vector<node_key> get_apices(const face_key& f)
    {
        std::vector<node_key> apices;
        simplex_set lk_f;
        link(f, lk_f);
        for(auto nit = lk_f.nodes_begin(); nit != lk_f.nodes_end(); nit++)
        {
            apices.push_back(*nit);
        }
        return apices;
    }
    
    ////////////////////
    // MESH FUNCTIONS //
    ////////////////////
public:
    
    template<typename Key>
    bool exists(const Key& k)
    {
        return mesh.exists(k);
    }
    
    void star(const node_key &n, simplex_set& set)
    {
        mesh.star(n, set);
    }
    
    void star(const edge_key &e, simplex_set& set)
    {
        mesh.star(e, set);
    }
    
    void star(const face_key &f, simplex_set& set)
    {
        mesh.star(f, set);
    }
    
    void star(const tet_key &t, simplex_set& set)
    {
        mesh.star(t, set);
    }
    
    void star(simplex_set &set_, simplex_set& set)
    {
        mesh.star(set_, set);
    }
    
    void closure(const node_key &n, simplex_set& set)
    {
        mesh.closure(n, set);
    }
    
    void closure(const edge_key &e, simplex_set& set)
    {
        mesh.closure(e, set);
    }
    
    void closure(const face_key &f, simplex_set& set)
    {
        mesh.closure(f, set);
    }
    
    void closure(const tet_key &t, simplex_set& set)
    {
        mesh.closure(t, set);
    }
    
    void closure(simplex_set &set_, simplex_set& set)
    {
        mesh.closure(set_, set);
    }
    
    template<typename Key>
    void link(const Key& k, simplex_set& set)
    {
        mesh.link(k, set);
    }
    
    /**
     * Ensures consistent orientation of all faces to the two tetrahedra which are in the star of f.
     */
    void orient_face(const face_key& fid)
    {
        if (is_boundary(fid))
        {
            simplex_set st_f;
            star(fid, st_f);
            mesh.orient_faces_consistently(*st_f.tetrahedra_begin());
        }
        else if (is_interface(fid))
        {
            simplex_set st_f;
            star(fid, st_f);
            int label = -100;
            for (auto tit = st_f.tetrahedra_begin(); tit != st_f.tetrahedra_end(); tit++)
            {
                int tl = get_label(*tit);
                
                if (tl > label)
                {
                    mesh.orient_faces_consistently(*tit);
                }
                label = tl;
            }
        }
    }
    
    node_key split(const edge_key & e)
    {
        std::map<tet_key, int> tt;
        simplex_set st_e;
        star(e, st_e);
        
        for (auto tit = st_e.tetrahedra_begin(); tit != st_e.tetrahedra_end(); tit++)
        {
            tt[*tit] = get_label(*tit);
        }
        
        std::map<tet_key, tet_key> new_tets;
        node_key n = mesh.split_edge_helper(e, new_tets);
        
        for (auto it = new_tets.begin(); it != new_tets.end(); it++)
        {
            set_label(it->first, tt[it->second]);
        }
        
        simplex_set st_n;
        star(n, st_n);
        st_n.insert(n);
        update(st_n);
        return n;
    }
    
    node_key split(const face_key& f)
    {
        std::map<tet_key, int> tt;
        simplex_set st_f;
        star(f, st_f);
        
        for (auto tit = st_f.tetrahedra_begin(); tit != st_f.tetrahedra_end(); tit++)
        {
            tt[*tit] = get_label(*tit);
        }
        
        std::map<tet_key, tet_key> new_tets;
        node_key n = mesh.split_face_helper(f, new_tets);
        
        for(auto it = new_tets.begin(); it != new_tets.end(); it++)
        {
            set_label(it->first, tt[it->second]);
        }
        
        simplex_set st_n;
        star(n, st_n);
        st_n.insert(n);
        update(st_n);
        return n;
    }
    
    node_key split(const tet_key& t)
    {
        int label = get(t).label;
        
        node_key n = mesh.split_tetrahedron(t);
        
        simplex_set st_n;
        star(n, st_n);
        for (auto tit = st_n.tetrahedra_begin(); tit != st_n.tetrahedra_end(); tit++)
        {
            set_label(*tit, label);
        }
        st_n.insert(n);
        update(st_n);
        return n;
    }
    
    node_key collapse(edge_key& e)
    {
        auto nodes = get_nodes(e);
#ifdef DEBUG
        assert(nodes[0] != NULL_NODE);
        assert(nodes[1] != NULL_NODE);
#endif
        node_key n = mesh.edge_collapse_helper(e, nodes[1], nodes[0]);
        if (n == (node_key)-1) {
            return NULL_NODE;
        }
        simplex_set st_n, cl_st_n;
        star(n, st_n);
        closure(st_n, cl_st_n);
        update(cl_st_n);
        return n;
    }
    
    node_key flip_32(const edge_key& e)
    {
#ifdef DEBUG
        assert(!is_interface(e) && !is_boundary(e));
#endif
        simplex_set lk_e;
        link(e, lk_e);
#ifdef DEBUG
        assert(lk_e.size_nodes() == 3);
#endif
        node_key n1 = *lk_e.nodes_begin();
        node_key n2 = split(e);
        edge_key e2 = get_edge(n1, n2);
#ifdef DEBUG
        assert(e2 != NULL_EDGE);
#endif
        node_key n3 = collapse(e2);
#ifdef DEBUG
        assert(n3 != NULL_NODE);
        assert(n1 == n3);
#endif
        return n3;
    }
    
    node_key flip_23(const face_key& f)
    {
#ifdef DEBUG
        assert(!is_interface(f) && !is_boundary(f));
#endif
        simplex_set lk_f;
        link(f, lk_f);
        node_key n1 = *lk_f.nodes_begin();
#ifdef DEBUG
        assert(lk_f.size_nodes() == 2);
#endif
        node_key n2 = split(f);
        edge_key e = get_edge(n1, n2);
#ifdef DEBUG
        assert(e != NULL_EDGE);
#endif
        node_key n3 = collapse(e);
#ifdef DEBUG
        assert(n3 != NULL_NODE);
        assert(n1 == n3);
#endif
        return n3;
    }
    
    node_key flip_22(const face_key& f1, const face_key& f2)
    {
        return flip_44(f1, f2);
    }
    
    node_key flip_44(const face_key& f1, const face_key& f2)
    {
#ifdef DEBUG
        assert((is_interface(f1) && is_interface(f2)) || (!is_interface(f1) && !is_interface(f2)));
        assert((is_boundary(f1) && is_boundary(f2)) || (!is_boundary(f1) && !is_boundary(f2)));
#endif
        edge_key e1 = get_edge(f1, f2);
        node_key n1 = get_apex(f1, e1);
        node_key n2 = split(e1);
        edge_key e2 = get_edge(n1, n2);
#ifdef DEBUG
        assert(e2 != NULL_EDGE);
#endif
        node_key n3 = collapse(e2);
#ifdef DEBUG
        assert(n3 != NULL_NODE);
        assert(n1 == n3);
#endif
        return n3;
    }
    
    ///////////////////////
    // UTILITY FUNCTIONS //
    ///////////////////////
    
    void garbage_collect()
    {
        mesh.garbage_collect();
    }
    
private:
    void check_validity()
    {
        std::cout << "Validity check" << std::endl;
        
        bool valid = true;
        
        for (auto fit = mesh.faces_begin(); fit != mesh.faces_end(); fit++)
        {
            simplex_set st_f;
            star(fit.key(), st_f);
            if (st_f.size_tetrahedra() > 2)
            {                
                std::cout << fit.key() << ": " << std::endl;
                for (auto tit = st_f.tetrahedra_begin(); tit != st_f.tetrahedra_end(); ++tit)
                {
                    simplex_set cl_t;
                    closure(*tit, cl_t);
                    
                    for (auto nit = cl_t.nodes_begin(); nit != cl_t.nodes_end(); ++nit)
                    {
                        std::cout << *nit << " ";
                    }
                    std::cout << std::endl;
                    
                }
                valid = false;
            }
        }
        
        if (!valid)
        {
            std::cout << "Input mesh invalid" << std::endl;
        }
        else
        {
            std::cout << "Input mesh valid" << std::endl;
        }
        assert(valid);
    }
};
