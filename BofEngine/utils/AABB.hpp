#pragma once

#include <algorithm>
#include <glm/glm.hpp>

/// Standalone axis aligned bounding box implemented built on top of GLM.
struct AABB
{
  AABB();
  AABB(const glm::vec3& center, float radius);
  AABB(const glm::vec3& p1, const glm::vec3& p2);
  AABB(const AABB& aabb);
  ~AABB() = default;
  void setNull() {m_min = glm::vec3(1.0); m_max = glm::vec3(-1.0);}

  void set(const glm::vec3& min, const glm::vec3& max) { m_min = min; m_max = max; }

  /// Returns true if AABB is NULL (not set).
  bool isNull() const {return m_min.x > m_max.x || m_min.y > m_max.y || m_min.z > m_max.z;}

  /// Extend the bounding box on all sides by \p val.
  void extend(float val);

  /// Expand the AABB to include point \p p.
  void extend(const glm::vec3& p);
  void extend(const glm::vec3& min, const glm::vec3& max) { extend(min); extend(max); }

  /// Expand the AABB to include a sphere centered at \p center and of radius \p
  /// radius.
  void extend(const glm::vec3& center, float radius);

  /// Expand the AABB to encompass the given \p aabb.
  void extend(const AABB& aabb);

  /// Expand the AABB to include a disk centered at \p center, with normal \p
  /// normal, and radius \p radius.
  void extendDisk(
      const glm::vec3& center, 
      const glm::vec3& normal,
      float radius);

  /// Translates AABB by vector \p v.
  void translate(const glm::vec3& v);

  /// Scale the AABB by \p scale, centered around \p origin.
  void scale(const glm::vec3& scale, const glm::vec3& origin);

  /// Retrieves the center of the AABB.
  glm::vec3 getCenter() const;

  /// Retrieves the diagonal vector (computed as mMax - mMin).
  /// If the AABB is NULL, then a vector of all zeros is returned.
  glm::vec3 getDiagonal() const;

  /// Retrieves the longest edge.
  /// If the AABB is NULL, then 0 is returned.
  float getLongestEdge() const;

  /// Retrieves the shortest edge.
  /// If the AABB is NULL, then 0 is returned.
  float getShortestEdge() const;

  /// Retrieves the AABB's minimum point.
  const glm::vec3& getMin() const {return m_min;}

  /// Retrieves the AABB's maximum point.
  const glm::vec3& getMax() const {return m_max;}

  /// Returns true if AABBs share a face overlap.
  bool overlaps(const AABB& bb) const;

  /// Type returned from call to intersect.
  enum INTERSECTION_TYPE { INSIDE, INTERSECT, OUTSIDE };
  /// Returns one of the intersection types. If either of the aabbs are invalid,
  /// then OUTSIDE is returned.
  INTERSECTION_TYPE intersect(const AABB& bb) const;

  glm::vec3 m_min;
  glm::vec3 m_max;
};

inline AABB::AABB()
{
  setNull();
}

inline AABB::AABB(const glm::vec3& center, float radius)
{
  setNull();
  extend(center, radius);
}

inline AABB::AABB(const glm::vec3& p1, const glm::vec3& p2)
{
  setNull();
  extend(p1);
  extend(p2);
}

inline AABB::AABB(const AABB& aabb)
{
  setNull();
  extend(aabb);
}

inline void AABB::extend(float val)
{
  if (!isNull())
  {
    m_min -= glm::vec3(val);
    m_max += glm::vec3(val);
  }
}

inline void AABB::extend(const glm::vec3& p)
{
  if (!isNull())
  {
    m_min = glm::min(p, m_min);
    m_max = glm::max(p, m_max);
  }
  else
  {
    m_min = p;
    m_max = p;
  }
}

inline void AABB::extend(const glm::vec3& p, float radius)
{
  glm::vec3 r(radius);
  if (!isNull())
  {
    m_min = glm::min(p - r, m_min);
    m_max = glm::max(p + r, m_max);
  }
  else
  {
    m_min = p - r;
    m_max = p + r;
  }
}

inline void AABB::extend(const AABB& aabb)
{
  if (!aabb.isNull())
  {
    extend(aabb.m_min);
    extend(aabb.m_max);
  }
}

inline void AABB::extendDisk(const glm::vec3& c, const glm::vec3& n, float r)
{
  if (glm::length(n) < 1.e-12) { extend(c); return; }
  glm::vec3 norm = glm::normalize(n);
  float x = sqrt(1 - norm.x) * r;
  float y = sqrt(1 - norm.y) * r;
  float z = sqrt(1 - norm.z) * r;
  extend(c + glm::vec3(x,y,z));
  extend(c - glm::vec3(x,y,z));
}

inline glm::vec3 AABB::getDiagonal() const
{
  if (!isNull())
    return m_max - m_min;
  else
    return glm::vec3(0);
}

inline float AABB::getLongestEdge() const
{
  glm::vec3 d = getDiagonal();
  return std::max(d.x, std::max(d.y, d.z));
}

inline float AABB::getShortestEdge() const
{
  glm::vec3 d = getDiagonal();
  return std::min(d.x, std::min(d.y, d.z));
}

inline glm::vec3 AABB::getCenter() const
{
  if (!isNull())
  {
    glm::vec3 d = getDiagonal();
    return m_min + (d * float(0.5f));
  }
  else
  {
    return glm::vec3(0.0);
  }
}

inline void AABB::translate(const glm::vec3& v)
{
  if (!isNull())
  {
    m_min += v;
    m_max += v;
  }
}

inline void AABB::scale(const glm::vec3& s, const glm::vec3& o)
{
  if (!isNull())
  {
    m_min -= o;
    m_max -= o;
    
    m_min *= s;
    m_max *= s;
    
    m_min += o;
    m_max += o;
  }
}

inline bool AABB::overlaps(const AABB& bb) const
{
  if (isNull() || bb.isNull())
    return false;
  
  if( bb.m_min.x > m_max.x || bb.m_max.x < m_min.x)
    return false;
  else if( bb.m_min.y > m_max.y || bb.m_max.y < m_min.y)
    return false;
  else if( bb.m_min.z > m_max.z || bb.m_max.z < m_min.z)
    return false;
  
  return true;
}

inline AABB::INTERSECTION_TYPE AABB::intersect(const AABB& b) const
{
  if (isNull() || b.isNull())
    return OUTSIDE;
  
  if ((m_max.x < b.m_min.x) || (m_min.x > b.m_max.x) ||
      (m_max.y < b.m_min.y) || (m_min.y > b.m_max.y) ||
      (m_max.z < b.m_min.z) || (m_min.z > b.m_max.z))
  {
    return OUTSIDE;
  }
  
  if ((m_min.x <= b.m_min.x) && (m_max.x >= b.m_max.x) &&
      (m_min.y <= b.m_min.y) && (m_max.y >= b.m_max.y) &&
      (m_min.z <= b.m_min.z) && (m_max.z >= b.m_max.z))
  {
    return INSIDE;
  }
  
  return INTERSECT;
}
