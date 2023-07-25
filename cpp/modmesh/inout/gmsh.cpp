#include <modmesh/inout/gmsh.hpp>

namespace modmesh
{
namespace inout
{
Gmsh::Gmsh(const std::string & data)
{
    bool meta_enter = false;
    bool node_enter = false;
    bool element_enter = false;
    // clang-format off
    std::unordered_map<std::string, std::function<void()>> keyword_handler = {
        {"$MeshFormat", [this, &meta_enter]() { load_meta(); meta_enter = true; }},
        {"$Nodes", [this, &node_enter]() { load_nodes(); node_enter = true; }},
        {"$Elements", [this, &element_enter]() { load_elements(); element_enter = true; }},
        {"$PhysicalNames", [this]() { load_physical(); }}};
    // clang-format on

    stream << data;

    std::string line;
    while (std::getline(stream, line))
    {
        // Using a finite state machine to check the input msh file format is valid or not.
        // $ is a keyword to trigger state transition.
        if (line.find('$') != std::string::npos)
        {
            auto it = keyword_handler.find(line);
            if (it != keyword_handler.end())
            {
                if (isValidTransition(it->first))
                {
                    it->second();
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    // MeshFormat, Nodes and Elements section is mandatory in gmsh msh file,
    // therefore need to check these sections are exist or not otherwise
    // modmesh may crash during mesh processing.
    if (!(meta_enter && node_enter && element_enter))
    {
        throw std::invalid_argument("Incorrect msh file format.");
    }
}

std::shared_ptr<StaticMesh> Gmsh::toblock()
{
    std::shared_ptr<StaticMesh> block = StaticMesh::construct(m_eldim.max(), m_nds.shape(0), 0, m_elems.size());
    build_interior(block);
    return block;
}

void Gmsh::build_interior(const std::shared_ptr<StaticMesh> & blk)
{
    blk->cltpn().swap(m_cltpn);
    blk->ndcrd().swap(m_nds);
    for (size_t i = 0; i < m_elems.size(); ++i)
    {
        blk->clnds()(i, 0) = m_elems[i][0];
        for (size_t j = 1; j <= m_elems[i][0]; ++j)
        {
            blk->clnds()(i, j) = m_elems[i][j];
        }
    }
    blk->build_interior(true);
    blk->build_boundary();
    blk->build_ghost();
}
} /* end namespace inout */
} /* end namespace modmesh */

// vim: set ff=unix fenc=utf8 et sw=4 ts=4 sts=4:
