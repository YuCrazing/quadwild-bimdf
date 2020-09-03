#ifndef VERTEX_CLASSIFIER
#define VERTEX_CLASSIFIER

#include "vert_field_graph.h"

#define CONVEX_THR 5.0
#define CONCAVE_THR 5.0
#define NARROW_THR 20.0

#define MAX_SAMPLES 1000
#define MAX_NARROW_CONST 0.05

enum TypeVert{TVNarrow,TVConcave,TVConvex,TVFlat,TVInternal,TVChoosen,TVNone};

template <class MeshType>
class VertexClassifier
{
    typedef typename MeshType::CoordType CoordType;
    typedef typename MeshType::ScalarType ScalarType;
    typedef typename MeshType::FaceType FaceType;
    typedef typename MeshType::VertexType VertexType;

public:

    static void FindConvexV(VertexFieldGraph<MeshType> &VFGraph,
                            std::vector<size_t> &ConvexV)
    {
        //find the convex on the original border
        ConvexV.clear();
        vcg::tri::UpdateSelection<MeshType>::VertexCornerBorder(VFGraph.Mesh(),
                                                                M_PI-M_PI/CONVEX_THR);
        for (size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            if (!VFGraph.Mesh().vert[i].IsS())continue;
            ConvexV.push_back(i);
        }
    }

    static void FindConcaveV(VertexFieldGraph<MeshType> &VFGraph,
                             std::vector<size_t> &ConcaveV)
    {
        //then find the concaves
        vcg::tri::UpdateSelection<MeshType>::VertexCornerBorder(VFGraph.Mesh(),M_PI+M_PI/CONCAVE_THR);
        for (size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            if (!VFGraph.Mesh().vert[i].IsB())continue;
            if (VFGraph.Mesh().vert[i].IsS())continue;
            ConcaveV.push_back(i);
        }
        std::sort(ConcaveV.begin(),ConcaveV.end());
        auto last=std::unique(ConcaveV.begin(),ConcaveV.end());
        ConcaveV.erase(last, ConcaveV.end());
    }

    static void FindNarrowV(VertexFieldGraph<MeshType> &VFGraph,
                            std::vector<size_t> &NarrowV)
    {
        //first cumulate the angle for each vertex
        std::map<CoordType,ScalarType> VertAngle;
        std::vector<ScalarType > angle(VFGraph.Mesh().vert.size(),0);

        for(size_t i=0;i<VFGraph.Mesh().face.size();i++)
        {
            for(size_t j=0;j<VFGraph.Mesh().face[i].VN();++j)
            {
                size_t IndexV=vcg::tri::Index(VFGraph.Mesh(),VFGraph.Mesh().face[i].V(j));
                angle[IndexV] += vcg::face::WedgeAngleRad(VFGraph.Mesh().face[i],j);
            }
        }

        //then cumulate across shared borders (considering that they have been splitted)
        for(size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            CoordType testP=VFGraph.Mesh().vert[i].P();
            if (VertAngle.count(testP)==0)
                VertAngle[testP]=angle[i];
            else
                VertAngle[testP]+=angle[i];
        }

        //then check the borders ont
        for(size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            if (!VFGraph.Mesh().vert[i].IsB())continue;
            ScalarType sideAngle=angle[i];
            if (VFGraph.IsRealBorderVert(i))
            {
                if(sideAngle>(2*M_PI-M_PI/NARROW_THR))
                    NarrowV.push_back(i);
            }
            else
            {
                CoordType testP=VFGraph.Mesh().vert[i].P();
                assert(VertAngle.count(testP)>0);
                ScalarType currAngle=VertAngle[testP];
                if (sideAngle>(currAngle-M_PI/NARROW_THR))
                    NarrowV.push_back(i);
            }
        }

        //finally add the one that are on a single border
        std::map<CoordType,size_t> BorderCount;
        for(size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            if (!VFGraph.Mesh().vert[i].IsB())continue;
            BorderCount[VFGraph.Mesh().vert[i].P()]++;
        }
        for(size_t i=0;i<VFGraph.Mesh().vert.size();i++)
        {
            if (!VFGraph.Mesh().vert[i].IsB())continue;
            if (BorderCount[VFGraph.Mesh().vert[i].P()]==1)NarrowV.push_back(i);
        }
        std::sort(NarrowV.begin(),NarrowV.end());
        auto last=std::unique(NarrowV.begin(),NarrowV.end());
        NarrowV.erase(last, NarrowV.end());
    }

    static void FindFlatV(VertexFieldGraph<MeshType> &VFGraph,
                          const std::vector<size_t> &ConvexV,
                          const std::vector<size_t> &ConcaveV,
                          const std::vector<size_t> &NarrowV,
                          std::vector<size_t> &FlatV)
    {
        FlatV.clear();

        vcg::tri::UpdateFlags<MeshType>::VertexClearV(VFGraph.Mesh());
        for (size_t i=0;i<ConvexV.size();i++)
            VFGraph.Mesh().vert[ConvexV[i]].SetV();

        for (size_t i=0;i<ConcaveV.size();i++)
            VFGraph.Mesh().vert[ConcaveV[i]].SetV();

        for (size_t i=0;i<NarrowV.size();i++)
            VFGraph.Mesh().vert[NarrowV[i]].SetV();

        for (size_t i=0;i<VFGraph.Mesh().face.size();i++)
            for (size_t j=0;j<3;j++)
            {
                bool IsB=vcg::face::IsBorder(VFGraph.Mesh().face[i],j);
                //bool IsSharp=VFGraph.Mesh().face[i].IsFaceEdgeS(j);
                if (!IsB)continue;

                VertexType *v0=VFGraph.Mesh().face[i].cV0(j);
                VertexType *v1=VFGraph.Mesh().face[i].cV1(j);
                size_t IndexV0=vcg::tri::Index(VFGraph.Mesh(),v0);
                size_t IndexV1=vcg::tri::Index(VFGraph.Mesh(),v1);

                if (!v0->IsV())FlatV.push_back(IndexV0);
                if (!v1->IsV())FlatV.push_back(IndexV1);
            }
        std::sort(FlatV.begin(),FlatV.end());
        auto last=std::unique(FlatV.begin(),FlatV.end());
        FlatV.erase(last, FlatV.end());
    }
};

#endif
