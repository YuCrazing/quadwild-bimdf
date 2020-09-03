#ifndef CANDIDATE_PATH
#define CANDIDATE_PATH


#include "vert_field_graph.h"
#include "graph_query.h"
#include "vertex_classifier.h"


enum TraceType{TraceDirect,DijkstraReceivers,TraceLoop};

struct CandidateTrace
{
    TypeVert FromType;
    TypeVert ToType;
    TraceType TracingMethod;
    size_t InitNode;
    std::vector<size_t> PathNodes;
    bool IsLoop;
    bool Updated;
    float Priority;

    inline bool operator <(const CandidateTrace &C1)const
    {return (Priority<C1.Priority);}

    CandidateTrace(){}

    CandidateTrace(TypeVert _FromType,
                   TypeVert _ToType,
                   TraceType _TracingMethod,
                   size_t _InitNode)
    {
        FromType=_FromType;
        ToType=_ToType;
        TracingMethod=_TracingMethod;
        InitNode=_InitNode;
        IsLoop=false;
        Updated=false;
        Priority=0;
    }
};

void GetCandidateNodes(const std::vector<CandidateTrace> &TraceSet,
                       std::vector<size_t> &ChoosenNodes)
{
    ChoosenNodes.clear();
    for (size_t i=0;i<TraceSet.size();i++)
        for (size_t j=0;j<TraceSet[i].PathNodes.size();j++)
        {
            size_t IndexN=TraceSet[i].PathNodes[j];
            ChoosenNodes.push_back(IndexN);
        }
}

template <class MeshType>
void GetCandidateNodesNodesAndTangent(const std::vector<CandidateTrace> &TraceSet,
                                      std::vector<size_t> &ChoosenNodes)
{
    GetCandidateNodes(TraceSet,ChoosenNodes);
    std::vector<size_t> TangentNodes=ChoosenNodes;
    VertexFieldGraph<MeshType>::TangentNodes(TangentNodes);
    ChoosenNodes.insert(ChoosenNodes.end(),TangentNodes.begin(),TangentNodes.end());
}

template <class MeshType>
void GetPathPos(VertexFieldGraph<MeshType> &VFGraph,
                const std::vector<CandidateTrace> &TraceSet,
                std::vector<std::vector<vcg::face::Pos<typename MeshType::FaceType> > > &Paths)
{
    typedef typename MeshType::FaceType FaceType;

    Paths.clear();
    Paths.resize(TraceSet.size());
    for (size_t i=0;i<TraceSet.size();i++)
    {
        VFGraph.GetNodesPos(TraceSet[i].PathNodes,
                            TraceSet[i].IsLoop,
                            Paths[i]);
    }
}

template <class MeshType>
bool CollideCandidates(const VertexFieldGraph<MeshType> &VFGraph,
                       const CandidateTrace &CT0,
                       const CandidateTrace &CT1)
{
    return (VertexFieldQuery<MeshType>::CollideTraces(VFGraph,CT0.PathNodes,CT0.IsLoop,
                                                      CT1.PathNodes,CT1.IsLoop));
}

template <class MeshType>
bool CollideWithCandidateSet(const VertexFieldGraph<MeshType> &VFGraph,
                             const CandidateTrace &TestTrace,
                             const std::vector<CandidateTrace> &TraceSet)
{
    for (size_t i=0;i<TraceSet.size();i++)
    {
        bool collide=CollideCandidates<MeshType>(VFGraph,TestTrace,TraceSet[i]);
        if (collide)return true;
    }
    return false;
}

template <class MeshType>
bool UpdateCandidate(VertexFieldGraph<MeshType> &VFGraph,
                     CandidateTrace &ToUpdate,
                     const typename MeshType::ScalarType &Drift,
                     const typename MeshType::ScalarType &MaxDijstraDist)
{
    assert(!ToUpdate.Updated);
    ToUpdate.Updated=true;

    size_t IndexN0=ToUpdate.InitNode;
    assert(VFGraph.IsActive(IndexN0));

    if (ToUpdate.TracingMethod==TraceDirect)
    {
        std::vector<size_t> PathN;
        bool hasTraced=TraceDirectPath(VFGraph,IndexN0,PathN);
        if (!hasTraced)return false;
        ToUpdate.PathNodes=PathN;
        ToUpdate.IsLoop=false;
        return true;
    }
    if (ToUpdate.TracingMethod==DijkstraReceivers)
    {
        std::vector<size_t> PathN;
        bool hasTraced=TraceDijkstraPath(VFGraph,IndexN0,Drift,MaxDijstraDist,PathN);
        if (!hasTraced)return false;
        ToUpdate.PathNodes=PathN;
        ToUpdate.IsLoop=false;
        return true;
    }
    if (ToUpdate.TracingMethod==TraceLoop)
    {
        std::vector<size_t> PathN;
        bool hasTraced=TraceLoopPath(VFGraph,IndexN0,Drift,PathN);
        if (!hasTraced)return false;
        ToUpdate.PathNodes=PathN;
        ToUpdate.IsLoop=true;
        return true;
    }
}



#endif
