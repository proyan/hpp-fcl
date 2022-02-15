/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011-2014, Willow Garage, Inc.
 *  Copyright (c) 2014-2016, Open Source Robotics Foundation
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Open Source Robotics Foundation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

/** @author Jia Pan */

#ifndef HPP_FCL_BROAD_PHASE_DYNAMIC_AABB_TREE_ARRAY_INL_H
#define HPP_FCL_BROAD_PHASE_DYNAMIC_AABB_TREE_ARRAY_INL_H

#include "hpp/fcl/broadphase/broadphase_dynamic_AABB_tree_array.h"

#if HPP_FCL_HAVE_OCTOMAP
#include "hpp/fcl/octree.h"
#endif
namespace hpp
{
namespace fcl
{
namespace detail
{
namespace dynamic_AABB_tree_array
{

#if HPP_FCL_HAVE_OCTOMAP

//==============================================================================
inline
HPP_FCL_DLLAPI
bool collisionRecurse_(
    DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1,
    size_t root1_id,
    const OcTree* tree2,
    const OcTree::OcTreeNode* root2,
    const AABB& root2_bv,
    const Transform3f& tf2,
    void* cdata,
    CollisionCallBack callback)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  if(!root2)
  {
    if(root1->isLeaf())
    {
      CollisionObject* obj1 = static_cast<CollisionObject*>(root1->data);

      if(!obj1->collisionGeometry()->isFree())
      {
        OBB obb1, obb2;
        convertBV(root1->bv, Transform3f::Identity(), obb1);
        convertBV(root2_bv, tf2, obb2);

        if(obb1.overlap(obb2))
        {
          Box* box = new Box();
          Transform3f box_tf;
          constructBox(root2_bv, tf2, *box, box_tf);

          box->cost_density = tree2->getDefaultOccupancy();

          CollisionObject obj2(boost::shared_ptr<CollisionGeometry>(box), box_tf);
          return callback(obj1, &obj2, cdata);
        }
      }
    }
    else
    {
      if(collisionRecurse_(nodes1, root1->children[0], tree2, nullptr, root2_bv, tf2, cdata, callback))
        return true;
      if(collisionRecurse_(nodes1, root1->children[1], tree2, nullptr, root2_bv, tf2, cdata, callback))
        return true;
    }

    return false;
  }
  else if(root1->isLeaf() && !tree2->nodeHasChildren(root2))
  {
    CollisionObject* obj1 = static_cast<CollisionObject*>(root1->data);
    if(!tree2->isNodeFree(root2) && !obj1->collisionGeometry()->isFree())
    {
      OBB obb1, obb2;
      convertBV(root1->bv, Transform3f::Identity(), obb1);
      convertBV(root2_bv, tf2, obb2);

      if(obb1.overlap(obb2))
      {
        Box* box = new Box();
        Transform3f box_tf;
        constructBox(root2_bv, tf2, *box, box_tf);

        box->cost_density = root2->getOccupancy();
        box->threshold_occupied = tree2->getOccupancyThres();

        CollisionObject obj2(boost::shared_ptr<CollisionGeometry>(box), box_tf);
        return callback(obj1, &obj2, cdata);
      }
      else return false;
    }
    else return false;
  }

  OBB obb1, obb2;
  convertBV(root1->bv, Transform3f::Identity(), obb1);
  convertBV(root2_bv, tf2, obb2);

  if(tree2->isNodeFree(root2) || !obb1.overlap(obb2)) return false;

  if(!tree2->nodeHasChildren(root2) || (!root1->isLeaf() && (root1->bv.size() > root2_bv.size())))
  {
    if(collisionRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, tf2, cdata, callback))
      return true;
    if(collisionRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, tf2, cdata, callback))
      return true;
  }
  else
  {
    for(unsigned int i = 0; i < 8; ++i)
    {
      if(tree2->nodeChildExists(root2, i))
      {
        const OcTree::OcTreeNode* child = tree2->getNodeChild(root2, i);
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);

        if(collisionRecurse_(nodes1, root1_id, tree2, child, child_bv, tf2, cdata, callback))
          return true;
      }
      else
      {
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);
        if(collisionRecurse_(nodes1, root1_id, tree2, nullptr, child_bv, tf2, cdata, callback))
          return true;
      }
    }
  }

  return false;
}

//==============================================================================
template <typename Derived>
HPP_FCL_DLLAPI
bool collisionRecurse_(
    DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1,
    size_t root1_id,
    const OcTree* tree2,
    const OcTree::OcTreeNode* root2,
    const AABB& root2_bv,
    const Eigen::MatrixBase<Derived>& translation2,
    void* cdata,
    CollisionCallBack callback)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  if(!root2)
  {
    if(root1->isLeaf())
    {
      CollisionObject* obj1 = static_cast<CollisionObject*>(root1->data);

      if(!obj1->collisionGeometry()->isFree())
      {
        const AABB& root_bv_t = translate(root2_bv, translation2);
        if(root1->bv.overlap(root_bv_t))
        {
          Box* box = new Box();
          Transform3f box_tf;
          Transform3f tf2 = Transform3f::Identity();
          tf2.translation() = translation2;
          constructBox(root2_bv, tf2, *box, box_tf);

          box->cost_density = tree2->getDefaultOccupancy();

          CollisionObject obj2(boost::shared_ptr<CollisionGeometry>(box), box_tf);
          return callback(obj1, &obj2, cdata);
        }
      }
    }
    else
    {
      if(collisionRecurse_(nodes1, root1->children[0], tree2, nullptr, root2_bv, translation2, cdata, callback))
        return true;
      if(collisionRecurse_(nodes1, root1->children[1], tree2, nullptr, root2_bv, translation2, cdata, callback))
        return true;
    }

    return false;
  }
  else if(root1->isLeaf() && !tree2->nodeHasChildren(root2))
  {
    CollisionObject* obj1 = static_cast<CollisionObject*>(root1->data);
    if(!tree2->isNodeFree(root2) && !obj1->collisionGeometry()->isFree())
    {
      const AABB& root_bv_t = translate(root2_bv, translation2);
      if(root1->bv.overlap(root_bv_t))
      {
        Box* box = new Box();
        Transform3f box_tf;
        Transform3f tf2 = Transform3f::Identity();
        tf2.translation() = translation2;
        constructBox(root2_bv, tf2, *box, box_tf);

        box->cost_density = root2->getOccupancy();
        box->threshold_occupied = tree2->getOccupancyThres();

        CollisionObject obj2(boost::shared_ptr<CollisionGeometry>(box), box_tf);
        return callback(obj1, &obj2, cdata);
      }
      else return false;
    }
    else return false;
  }

  const AABB& root_bv_t = translate(root2_bv, translation2);
  if(tree2->isNodeFree(root2) || !root1->bv.overlap(root_bv_t)) return false;

  if(!tree2->nodeHasChildren(root2) || (!root1->isLeaf() && (root1->bv.size() > root2_bv.size())))
  {
    if(collisionRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, translation2, cdata, callback))
      return true;
    if(collisionRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, translation2, cdata, callback))
      return true;
  }
  else
  {
    for(unsigned int i = 0; i < 8; ++i)
    {
      if(tree2->nodeChildExists(root2, i))
      {
        const OcTree::OcTreeNode* child = tree2->getNodeChild(root2, i);
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);

        if(collisionRecurse_(nodes1, root1_id, tree2, child, child_bv, translation2, cdata, callback))
          return true;
      }
      else
      {
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);
        if(collisionRecurse_(nodes1, root1_id, tree2, nullptr, child_bv, translation2, cdata, callback))
          return true;
      }
    }
  }

  return false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool distanceRecurse_(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1, size_t root1_id, const OcTree* tree2, const OcTree::OcTreeNode* root2, const AABB& root2_bv, const Transform3f& tf2, void* cdata, DistanceCallBack callback, FCL_REAL& min_dist)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  if(root1->isLeaf() && !tree2->nodeHasChildren(root2))
  {
    if(tree2->isNodeOccupied(root2))
    {
      Box* box = new Box();
      Transform3f box_tf;
      constructBox(root2_bv, tf2, *box, box_tf);
      CollisionObject obj(boost::shared_ptr<CollisionGeometry>(box), box_tf);
      return callback(static_cast<CollisionObject*>(root1->data), &obj, cdata, min_dist);
    }
    else return false;
  }

  if(!tree2->isNodeOccupied(root2)) return false;

  if(!tree2->nodeHasChildren(root2) || (!root1->isLeaf() && (root1->bv.size() > root2_bv.size())))
  {
    AABB aabb2;
    convertBV(root2_bv, tf2, aabb2);

    FCL_REAL d1 = aabb2.distance((nodes1 + root1->children[0])->bv);
    FCL_REAL d2 = aabb2.distance((nodes1 + root1->children[1])->bv);

    if(d2 < d1)
    {
      if(d2 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, tf2, cdata, callback, min_dist))
          return true;
      }

      if(d1 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, tf2, cdata, callback, min_dist))
          return true;
      }
    }
    else
    {
      if(d1 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, tf2, cdata, callback, min_dist))
          return true;
      }

      if(d2 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, tf2, cdata, callback, min_dist))
          return true;
      }
    }
  }
  else
  {
    for(unsigned int i = 0; i < 8; ++i)
    {
      if(tree2->nodeChildExists(root2, i))
      {
        const OcTree::OcTreeNode* child = tree2->getNodeChild(root2, i);
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);

        AABB aabb2;
        convertBV(child_bv, tf2, aabb2);
        FCL_REAL d = root1->bv.distance(aabb2);

        if(d < min_dist)
        {
          if(distanceRecurse_(nodes1, root1_id, tree2, child, child_bv, tf2, cdata, callback, min_dist))
            return true;
        }
      }
    }
  }

  return false;
}

//==============================================================================
template <typename Derived>
HPP_FCL_DLLAPI
bool distanceRecurse_(
    DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1,
    size_t root1_id,
    const OcTree* tree2,
    const OcTree::OcTreeNode* root2,
    const AABB& root2_bv,
    const Eigen::MatrixBase<Derived>& translation2,
    void* cdata,
    DistanceCallBack callback,
    FCL_REAL& min_dist)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  if(root1->isLeaf() && !tree2->nodeHasChildren(root2))
  {
    if(tree2->isNodeOccupied(root2))
    {
      Box* box = new Box();
      Transform3f box_tf;
      Transform3f tf2 = Transform3f::Identity();
      tf2.translation() = translation2;
      constructBox(root2_bv, tf2, *box, box_tf);
      CollisionObject obj(boost::shared_ptr<CollisionGeometry>(box), box_tf);
      return callback(static_cast<CollisionObject*>(root1->data), &obj, cdata, min_dist);
    }
    else return false;
  }

  if(!tree2->isNodeOccupied(root2)) return false;

  if(!tree2->nodeHasChildren(root2) || (!root1->isLeaf() && (root1->bv.size() > root2_bv.size())))
  {
    const AABB& aabb2 = translate(root2_bv, translation2);

    FCL_REAL d1 = aabb2.distance((nodes1 + root1->children[0])->bv);
    FCL_REAL d2 = aabb2.distance((nodes1 + root1->children[1])->bv);

    if(d2 < d1)
    {
      if(d2 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, translation2, cdata, callback, min_dist))
          return true;
      }

      if(d1 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, translation2, cdata, callback, min_dist))
          return true;
      }
    }
    else
    {
      if(d1 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[0], tree2, root2, root2_bv, translation2, cdata, callback, min_dist))
          return true;
      }

      if(d2 < min_dist)
      {
        if(distanceRecurse_(nodes1, root1->children[1], tree2, root2, root2_bv, translation2, cdata, callback, min_dist))
          return true;
      }
    }
  }
  else
  {
    for(unsigned int i = 0; i < 8; ++i)
    {
      if(tree2->nodeChildExists(root2, i))
      {
        const OcTree::OcTreeNode* child = tree2->getNodeChild(root2, i);
        AABB child_bv;
        computeChildBV(root2_bv, i, child_bv);

        const AABB& aabb2 = translate(child_bv, translation2);
        FCL_REAL d = root1->bv.distance(aabb2);

        if(d < min_dist)
        {
          if(distanceRecurse_(nodes1, root1_id, tree2, child, child_bv, translation2, cdata, callback, min_dist))
            return true;
        }
      }
    }
  }

  return false;
}


#endif

//==============================================================================
inline
HPP_FCL_DLLAPI
bool collisionRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1, size_t root1_id,
                      DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes2, size_t root2_id,
                      void* cdata, CollisionCallBack callback)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root2 = nodes2 + root2_id;
  if(root1->isLeaf() && root2->isLeaf())
  {
    if(!root1->bv.overlap(root2->bv)) return false;
    return callback(static_cast<CollisionObject*>(root1->data), static_cast<CollisionObject*>(root2->data), cdata);
  }

  if(!root1->bv.overlap(root2->bv)) return false;

  if(root2->isLeaf() || (!root1->isLeaf() && (root1->bv.size() > root2->bv.size())))
  {
    if(collisionRecurse(nodes1, root1->children[0], nodes2, root2_id, cdata, callback))
      return true;
    if(collisionRecurse(nodes1, root1->children[1], nodes2, root2_id, cdata, callback))
      return true;
  }
  else
  {
    if(collisionRecurse(nodes1, root1_id, nodes2, root2->children[0], cdata, callback))
      return true;
    if(collisionRecurse(nodes1, root1_id, nodes2, root2->children[1], cdata, callback))
      return true;
  }
  return false;
}

//==============================================================================
inline HPP_FCL_DLLAPI
bool collisionRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes, size_t root_id, CollisionObject* query, void* cdata, CollisionCallBack callback)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root = nodes + root_id;
  if(root->isLeaf())
  {
    if(!root->bv.overlap(query->getAABB())) return false;
    return callback(static_cast<CollisionObject*>(root->data), query, cdata);
  }

  if(!root->bv.overlap(query->getAABB())) return false;

  int select_res = implementation_array::select(query->getAABB(), root->children[0], root->children[1], nodes);

  if(collisionRecurse(nodes, root->children[select_res], query, cdata, callback))
    return true;

  if(collisionRecurse(nodes, root->children[1-select_res], query, cdata, callback))
    return true;

  return false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool selfCollisionRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes, size_t root_id, void* cdata, CollisionCallBack callback)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root = nodes + root_id;
  if(root->isLeaf()) return false;

  if(selfCollisionRecurse(nodes, root->children[0], cdata, callback))
    return true;

  if(selfCollisionRecurse(nodes, root->children[1], cdata, callback))
    return true;

  if(collisionRecurse(nodes, root->children[0], nodes, root->children[1], cdata, callback))
    return true;

  return false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool distanceRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1, size_t root1_id,
                     DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes2, size_t root2_id,
                     void* cdata, DistanceCallBack callback, FCL_REAL& min_dist)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root1 = nodes1 + root1_id;
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root2 = nodes2 + root2_id;
  if(root1->isLeaf() && root2->isLeaf())
  {
    CollisionObject* root1_obj = static_cast<CollisionObject*>(root1->data);
    CollisionObject* root2_obj = static_cast<CollisionObject*>(root2->data);
    return callback(root1_obj, root2_obj, cdata, min_dist);
  }

  if(root2->isLeaf() || (!root1->isLeaf() && (root1->bv.size() > root2->bv.size())))
  {
    FCL_REAL d1 = root2->bv.distance((nodes1 + root1->children[0])->bv);
    FCL_REAL d2 = root2->bv.distance((nodes1 + root1->children[1])->bv);

    if(d2 < d1)
    {
      if(d2 < min_dist)
      {
        if(distanceRecurse(nodes1, root1->children[1], nodes2, root2_id, cdata, callback, min_dist))
          return true;
      }

      if(d1 < min_dist)
      {
        if(distanceRecurse(nodes1, root1->children[0], nodes2, root2_id, cdata, callback, min_dist))
          return true;
      }
    }
    else
    {
      if(d1 < min_dist)
      {
        if(distanceRecurse(nodes1, root1->children[0], nodes2, root2_id, cdata, callback, min_dist))
          return true;
      }

      if(d2 < min_dist)
      {
        if(distanceRecurse(nodes1, root1->children[1], nodes2, root2_id, cdata, callback, min_dist))
          return true;
      }
    }
  }
  else
  {
    FCL_REAL d1 = root1->bv.distance((nodes2 + root2->children[0])->bv);
    FCL_REAL d2 = root1->bv.distance((nodes2 + root2->children[1])->bv);

    if(d2 < d1)
    {
      if(d2 < min_dist)
      {
        if(distanceRecurse(nodes1, root1_id, nodes2, root2->children[1], cdata, callback, min_dist))
          return true;
      }

      if(d1 < min_dist)
      {
        if(distanceRecurse(nodes1, root1_id, nodes2, root2->children[0], cdata, callback, min_dist))
          return true;
      }
    }
    else
    {
      if(d1 < min_dist)
      {
        if(distanceRecurse(nodes1, root1_id, nodes2, root2->children[0], cdata, callback, min_dist))
          return true;
      }

      if(d2 < min_dist)
      {
        if(distanceRecurse(nodes1, root1_id, nodes2, root2->children[1], cdata, callback, min_dist))
          return true;
      }
    }
  }

  return false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool distanceRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes, size_t root_id, CollisionObject* query, void* cdata, DistanceCallBack callback, FCL_REAL& min_dist)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root = nodes + root_id;
  if(root->isLeaf())
  {
    CollisionObject* root_obj = static_cast<CollisionObject*>(root->data);
    return callback(root_obj, query, cdata, min_dist);
  }

  FCL_REAL d1 = query->getAABB().distance((nodes + root->children[0])->bv);
  FCL_REAL d2 = query->getAABB().distance((nodes + root->children[1])->bv);

  if(d2 < d1)
  {
    if(d2 < min_dist)
    {
      if(distanceRecurse(nodes, root->children[1], query, cdata, callback, min_dist))
        return true;
    }

    if(d1 < min_dist)
    {
      if(distanceRecurse(nodes, root->children[0], query, cdata, callback, min_dist))
        return true;
    }
  }
  else
  {
    if(d1 < min_dist)
    {
      if(distanceRecurse(nodes, root->children[0], query, cdata, callback, min_dist))
        return true;
    }

    if(d2 < min_dist)
    {
      if(distanceRecurse(nodes, root->children[1], query, cdata, callback, min_dist))
        return true;
    }
  }

  return false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool selfDistanceRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes, size_t root_id, void* cdata, DistanceCallBack callback, FCL_REAL& min_dist)
{
  DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* root = nodes + root_id;
  if(root->isLeaf()) return false;

  if(selfDistanceRecurse(nodes, root->children[0], cdata, callback, min_dist))
    return true;

  if(selfDistanceRecurse(nodes, root->children[1], cdata, callback, min_dist))
    return true;

  if(distanceRecurse(nodes, root->children[0], nodes, root->children[1], cdata, callback, min_dist))
    return true;

  return false;
}


#if HPP_FCL_HAVE_OCTOMAP

//==============================================================================
inline
HPP_FCL_DLLAPI
bool collisionRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1, size_t root1_id, const OcTree* tree2, const OcTree::OcTreeNode* root2, const AABB& root2_bv, const Transform3f& tf2, void* cdata, CollisionCallBack callback)
{
  if(tf2.rotation().isIdentity())
    return collisionRecurse_(nodes1, root1_id, tree2, root2, root2_bv, tf2.translation(), cdata, callback);
  else
    return collisionRecurse_(nodes1, root1_id, tree2, root2, root2_bv, tf2, cdata, callback);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool distanceRecurse(DynamicAABBTreeCollisionManager_Array::DynamicAABBNode* nodes1, size_t root1_id, const OcTree* tree2, const OcTree::OcTreeNode* root2, const AABB& root2_bv, const Transform3f& tf2, void* cdata, DistanceCallBack callback, FCL_REAL& min_dist)
{
  if(tf2.rotation().isIdentity())
    return distanceRecurse_(nodes1, root1_id, tree2, root2, root2_bv, tf2.translation(), cdata, callback, min_dist);
  else
    return distanceRecurse_(nodes1, root1_id, tree2, root2, root2_bv, tf2, cdata, callback, min_dist);
}

#endif

} // namespace dynamic_AABB_tree_array

} // namespace detail

//==============================================================================
inline
HPP_FCL_DLLAPI
DynamicAABBTreeCollisionManager_Array::DynamicAABBTreeCollisionManager_Array()
  : tree_topdown_balance_threshold(dtree.bu_threshold),
    tree_topdown_level(dtree.topdown_level)
{
  max_tree_nonbalanced_level = 10;
  tree_incremental_balance_pass = 10;
  tree_topdown_balance_threshold = 2;
  tree_topdown_level = 0;
  tree_init_level = 0;
  setup_ = false;

  // from experiment, this is the optimal setting
  octree_as_geometry_collide = true;
  octree_as_geometry_distance = false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::registerObjects(
    const std::vector<CollisionObject*>& other_objs)
{
  if(other_objs.empty()) return;

  if(size() > 0)
  {
    BroadPhaseCollisionManager::registerObjects(other_objs);
  }
  else
  {
    DynamicAABBNode* leaves = new DynamicAABBNode[other_objs.size()];
    table.rehash(other_objs.size());
    for(size_t i = 0, size = other_objs.size(); i < size; ++i)
    {
      leaves[i].bv = other_objs[i]->getAABB();
      leaves[i].parent = dtree.NULL_NODE;
      leaves[i].children[1] = dtree.NULL_NODE;
      leaves[i].data = other_objs[i];
      table[other_objs[i]] = i;
    }

    int n_leaves = other_objs.size();

    dtree.init(leaves, n_leaves, tree_init_level);

    setup_ = true;
  }
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::registerObject(CollisionObject* obj)
{
  size_t node = dtree.insert(obj->getAABB(), obj);
  table[obj] = node;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::unregisterObject(CollisionObject* obj)
{
  size_t node = table[obj];
  table.erase(obj);
  dtree.remove(node);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::setup()
{
  if(!setup_)
  {
    int num = dtree.size();
    if(num == 0)
    {
      setup_ = true;
      return;
    }

    int height = dtree.getMaxHeight();


    if(height - std::log((FCL_REAL)num) / std::log(2.0) < max_tree_nonbalanced_level)
      dtree.balanceIncremental(tree_incremental_balance_pass);
    else
      dtree.balanceTopdown();

    setup_ = true;
  }
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::update()
{
  for(auto it = table.cbegin(), end = table.cend(); it != end; ++it)
  {
    const CollisionObject* obj = it->first;
    size_t node = it->second;
    dtree.getNodes()[node].bv = obj->getAABB();
  }

  dtree.refit();
  setup_ = false;

  setup();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::update_(CollisionObject* updated_obj)
{
  const auto it = table.find(updated_obj);
  if(it != table.end())
  {
    size_t node = it->second;
    if(!(dtree.getNodes()[node].bv == updated_obj->getAABB()))
      dtree.update(node, updated_obj->getAABB());
  }
  setup_ = false;
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::update(CollisionObject* updated_obj)
{
  update_(updated_obj);
  setup();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::update(const std::vector<CollisionObject*>& updated_objs)
{
  for(size_t i = 0, size = updated_objs.size(); i < size; ++i)
    update_(updated_objs[i]);
  setup();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::clear()
{
  dtree.clear();
  table.clear();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::getObjects(std::vector<CollisionObject*>& objs) const
{
  objs.resize(this->size());
  std::transform(table.begin(), table.end(), objs.begin(), std::bind(&DynamicAABBTable::value_type::first, std::placeholders::_1));
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::collide(CollisionObject* obj, void* cdata, CollisionCallBack callback) const
{
  if(size() == 0) return;
  switch(obj->collisionGeometry()->getNodeType())
  {
#if HPP_FCL_HAVE_OCTOMAP
  case GEOM_OCTREE:
    {
      if(!octree_as_geometry_collide)
      {
        const OcTree* octree = static_cast<const OcTree*>(obj->collisionGeometry().get());
        detail::dynamic_AABB_tree_array::collisionRecurse(dtree.getNodes(), dtree.getRoot(), octree, octree->getRoot(), octree->getRootBV(), obj->getTransform(), cdata, callback);
      }
      else
        detail::dynamic_AABB_tree_array::collisionRecurse(dtree.getNodes(), dtree.getRoot(), obj, cdata, callback);
    }
    break;
#endif
  default:
    detail::dynamic_AABB_tree_array::collisionRecurse(dtree.getNodes(), dtree.getRoot(), obj, cdata, callback);
  }
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::distance(CollisionObject* obj, void* cdata, DistanceCallBack callback) const
{
  if(size() == 0) return;
  FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();
  switch(obj->collisionGeometry()->getNodeType())
  {
#if HPP_FCL_HAVE_OCTOMAP
  case GEOM_OCTREE:
    {
      if(!octree_as_geometry_distance)
      {
        const OcTree* octree = static_cast<const OcTree*>(obj->collisionGeometry().get());
        detail::dynamic_AABB_tree_array::distanceRecurse(dtree.getNodes(), dtree.getRoot(), octree, octree->getRoot(), octree->getRootBV(), obj->getTransform(), cdata, callback, min_dist);
      }
      else
        detail::dynamic_AABB_tree_array::distanceRecurse(dtree.getNodes(), dtree.getRoot(), obj, cdata, callback, min_dist);
    }
    break;
#endif
  default:
    detail::dynamic_AABB_tree_array::distanceRecurse(dtree.getNodes(), dtree.getRoot(), obj, cdata, callback, min_dist);
  }
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::collide(void* cdata, CollisionCallBack callback) const
{
  if(size() == 0) return;
  detail::dynamic_AABB_tree_array::selfCollisionRecurse(dtree.getNodes(), dtree.getRoot(), cdata, callback);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::distance(void* cdata, DistanceCallBack callback) const
{
  if(size() == 0) return;
  FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();
  detail::dynamic_AABB_tree_array::selfDistanceRecurse(dtree.getNodes(), dtree.getRoot(), cdata, callback, min_dist);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::collide(BroadPhaseCollisionManager* other_manager_, void* cdata, CollisionCallBack callback) const
{
  DynamicAABBTreeCollisionManager_Array* other_manager = static_cast<DynamicAABBTreeCollisionManager_Array*>(other_manager_);
  if((size() == 0) || (other_manager->size() == 0)) return;
  detail::dynamic_AABB_tree_array::collisionRecurse(dtree.getNodes(), dtree.getRoot(), other_manager->dtree.getNodes(), other_manager->dtree.getRoot(), cdata, callback);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
void DynamicAABBTreeCollisionManager_Array::distance(BroadPhaseCollisionManager* other_manager_, void* cdata, DistanceCallBack callback) const
{
  DynamicAABBTreeCollisionManager_Array* other_manager = static_cast<DynamicAABBTreeCollisionManager_Array*>(other_manager_);
  if((size() == 0) || (other_manager->size() == 0)) return;
  FCL_REAL min_dist = std::numeric_limits<FCL_REAL>::max();
  detail::dynamic_AABB_tree_array::distanceRecurse(dtree.getNodes(), dtree.getRoot(), other_manager->dtree.getNodes(), other_manager->dtree.getRoot(), cdata, callback, min_dist);
}

//==============================================================================
inline
HPP_FCL_DLLAPI
bool DynamicAABBTreeCollisionManager_Array::empty() const
{
  return dtree.empty();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
size_t DynamicAABBTreeCollisionManager_Array::size() const
{
  return dtree.size();
}

//==============================================================================
inline
HPP_FCL_DLLAPI
const detail::implementation_array::HierarchyTree<AABB>&
DynamicAABBTreeCollisionManager_Array::getTree() const
{
  return dtree;
}

} // namespace fcl

} // namespace hpp

#endif
