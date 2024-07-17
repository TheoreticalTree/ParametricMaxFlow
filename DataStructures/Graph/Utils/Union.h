#pragma once

#include <type_traits>

#include "../../Attributes/Attributes.h"

#include "../../../Helpers/Types.h"
#include "../../../Helpers/Ranges/Range.h"
#include "../../../Helpers/Ranges/EdgeUnionRange.h"

namespace Graph {

template<typename GRAPH_A, typename GRAPH_B>
class Union {

public:
    using GraphA = GRAPH_A;
    using GraphB = GRAPH_B;
    using Type = Union<GraphA, GraphB>;

    template<AttributeNameType ATTRIBUTE_NAME>
    using AttributeType = typename GraphA::template AttributeType<ATTRIBUTE_NAME>;

    template<AttributeNameType ATTRIBUTE_NAME>
    using AttributeConstReferenceType = typename GraphA::template AttributeConstReferenceType<ATTRIBUTE_NAME>;

public:
    Union(const GraphA& graphA, const GraphB& graphB) {
        setGraphs(graphA, graphB);
    }

    Union(const GraphA&&, const GraphB&) = delete;
    Union(const GraphA&, const GraphB&&) = delete;

    Union(const Type&) = default;
    Union(Type&&) = default;

    Type& operator=(const Type&) = default;
    Type& operator=(Type&&) = default;

    inline void setGraphs(const GraphA& graphA, const GraphB& graphB) noexcept {
        this->graphA = &graphA;
        this->graphB = &graphB;
        offset = Edge(graphA.edgeLimit());
        Assert(checkGraph(graphA), "Edges of graphA are not sorted!");
        Assert(checkGraph(graphB), "Edges of graphB are not sorted!");
        if constexpr (std::is_same_v<GraphA, GraphB>) {
            if (graphA.numVertices() > graphB.numVertices()) {
                std::swap(this->graphA, this->graphB);
                offset = Edge(graphA.edgeLimit());
            }
        }
    }

    inline size_t numVertices() const noexcept {
        if constexpr (std::is_same_v<GraphA, GraphB>) {
            return graphA->numVertices();
        } else {
            return std::max(graphA->numVertices(), graphB->numVertices());
        }
    }

    inline size_t edgeLimit() const noexcept {
        if constexpr (std::is_same_v<GraphA, GraphB>) {
            return graphA->edgeLimit();
        } else {
            return (graphA->numVertices() >= graphB->numVertices()) ? graphA->edgeLimit() : graphB->edgeLimit();
        }
    }

    inline bool isVertex(const Vertex vertex) const noexcept {
        return vertex < numVertices();
    }

    inline Range<Vertex> vertices() const noexcept {
        return Range<Vertex>(Vertex(0), Vertex(numVertices()));
    }

    inline EdgeUnionRange<GraphA, GraphB> edgesFrom(const Vertex vertex) const noexcept {
        Assert(isVertex(vertex), vertex << " is not a valid vertex!");
        return EdgeUnionRange<GraphA, GraphB>(graphA, graphB, offset, vertex);
    }

    inline Edge findEdge(const Vertex from, const Vertex to) const noexcept {
        const Edge edge = graphA->findEdge(from, to);
        if (edge == noEdge) {
            return graphB->findEdge(from, to) + offset;
        } else {
            return edge;
        }
    }

    inline bool hasEdge(const Vertex from, const Vertex to) const noexcept {
        return graphA->hasEdge(from, to) || graphB->hasEdge(from, to);
    }

    inline bool empty() const noexcept {
        return numVertices() == 0;
    }

    template<AttributeNameType ATTRIBUTE_NAME>
    inline AttributeConstReferenceType<ATTRIBUTE_NAME> get(const AttributeNameWrapper<ATTRIBUTE_NAME> attributeName, const Vertex vertex) const noexcept {
        if constexpr (std::is_same_v<GraphA, GraphB>) {
            return graphA->get(attributeName, vertex);
        } else {
            return (graphA->numVertices() >= graphB->numVertices()) ? graphA->get(attributeName, vertex) : graphB->get(attributeName, vertex);
        }
    }

    template<AttributeNameType ATTRIBUTE_NAME>
    inline AttributeConstReferenceType<ATTRIBUTE_NAME> get(const AttributeNameWrapper<ATTRIBUTE_NAME> attributeName, const Edge edge) const noexcept {
        return (edge < offset) ? graphA->get(attributeName, edge) : graphB->get(attributeName, edge - offset);
    }

private:
    template<typename GRAPH>
    inline bool checkGraph(const GRAPH& graph) const noexcept {
        for (const Vertex vertex : graph.vertices()) {
            Vertex current = Vertex(0);
            for (const Edge edge : graph.edgesFrom(vertex)) {
                if (current > graph.get(ToVertex, edge)) {
                    Assert(false, "Edges at vertex " << vertex << " are not sorted!");
                    return false;
                }
                current = graph.get(ToVertex, edge);
            }
        }
        return true;
    }

private:
    const GraphA* graphA;
    const GraphB* graphB;

    Edge offset;

};

template<typename GRAPH_A, typename GRAPH_B>
inline Union<GRAPH_A, GRAPH_B> makeUnion(const GRAPH_A& graphA, const GRAPH_B& graphB) noexcept {
    return Union<GRAPH_A, GRAPH_B> (graphA, graphB);
}

template<typename GRAPH_A, typename GRAPH_B>
inline Union<GRAPH_A, GRAPH_B> makeUnion(const GRAPH_A&& graphA, const GRAPH_B& graphB) = delete;

template<typename GRAPH_A, typename GRAPH_B>
inline Union<GRAPH_A, GRAPH_B> makeUnion(const GRAPH_A& graphA, const GRAPH_B&& graphB) = delete;

}
