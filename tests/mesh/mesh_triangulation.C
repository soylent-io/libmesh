#include <libmesh/elem.h>
#include <libmesh/mesh.h>
#include <libmesh/mesh_triangle_holes.h>
#include <libmesh/mesh_triangle_interface.h>
#include <libmesh/point.h>
#include <libmesh/poly2tri_triangulator.h>

#include "test_comm.h"
#include "libmesh_cppunit.h"

#include <cmath>


using namespace libMesh;

class MeshTriangulationTest : public CppUnit::TestCase
{
  /**
   * The goal of this test is to verify proper operation of the
   * interfaces to triangulation libraries
   */
public:
  CPPUNIT_TEST_SUITE( MeshTriangulationTest );

  CPPUNIT_TEST( testTriangleHoleArea );

#ifdef LIBMESH_HAVE_POLY2TRI
  CPPUNIT_TEST( testPoly2Tri );
#endif

#ifdef LIBMESH_HAVE_TRIANGLE
  CPPUNIT_TEST( testTriangle );
#endif

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {}

  void tearDown() {}

  void testTriangleHoleArea()
  {
    // Using center=(1,0), radius=2 for the heck of it
    Point center{1};
    Real radius = 2;
    std::vector<TriangulatorInterface::PolygonHole> polyholes;

    // Line
    polyholes.emplace_back(center, radius, 2);
    // Triangle
    polyholes.emplace_back(center, radius, 3);
    // Square
    polyholes.emplace_back(center, radius, 4);
    // Pentagon
    polyholes.emplace_back(center, radius, 5);
    // Hexagon
    polyholes.emplace_back(center, radius, 6);

    for (int i=0; i != 5; ++i)
      {
        const int n_sides = i+2;
        const TriangulatorInterface::Hole & hole = polyholes[i];

        // Really?  This isn't until C++20?
        constexpr double my_pi = 3.141592653589793238462643383279;

        const Real computed_area = hole.area();
        const Real theta = my_pi/n_sides;
        const Real half_side_length = radius*std::cos(theta);
        const Real apothem = radius*std::sin(theta);
        const Real area = n_sides * apothem * half_side_length;

        LIBMESH_ASSERT_FP_EQUAL(computed_area, area, TOLERANCE*TOLERANCE);
      }

    TriangulatorInterface::ArbitraryHole arbhole {center, {{0,-1},{2,-1},{2,1},{0,2}}};
    LIBMESH_ASSERT_FP_EQUAL(arbhole.area(), Real(5), TOLERANCE*TOLERANCE);

#ifdef LIBMESH_HAVE_TRIANGLE
    // Make sure we're compatible with the old naming structure too
    TriangleInterface::PolygonHole square(center, radius, 4);
    LIBMESH_ASSERT_FP_EQUAL(square.area(), 2*radius*radius, TOLERANCE*TOLERANCE);
#endif
  }

  void testTriangulator(MeshBase & mesh,
                        TriangulatorInterface & triangulator)
  {
    // A non-square quad, so we don't have ambiguity about which
    // diagonal a Delaunay algorithm will pick.
    mesh.add_point(Point(0,0));
    mesh.add_point(Point(1,0));
    mesh.add_point(Point(1,2));
    mesh.add_point(Point(0,1));

    // Use the point order to define the boundary, because our
    // Poly2Tri implementation doesn't do convex hulls yet, even when
    // that would give the same answer.
    triangulator.triangulation_type() = TriangulatorInterface::PSLG;

    // Don't try to insert points yet
    triangulator.desired_area() = 1000;
    triangulator.minimum_angle() = 0;
    triangulator.smooth_after_generating() = false;

    triangulator.triangulate();

    CPPUNIT_ASSERT_EQUAL(mesh.n_elem(), dof_id_type(2));
    for (const auto & elem : mesh.element_ptr_range())
      {
        CPPUNIT_ASSERT_EQUAL(elem->type(), TRI3);

        bool found_triangle = false;
        for (const auto & node : elem->node_ref_range())
          {
            const Point & point = node;
            if (point == Point(0,0))
              {
                found_triangle = true;
                CPPUNIT_ASSERT((elem->point(0) == Point(0,0) &&
                                elem->point(1) == Point(1,0) &&
                                elem->point(2) == Point(0,1)) ||
                               (elem->point(1) == Point(0,0) &&
                                elem->point(2) == Point(1,0) &&
                                elem->point(0) == Point(0,1)) ||
                               (elem->point(2) == Point(0,0) &&
                                elem->point(0) == Point(1,0) &&
                                elem->point(1) == Point(0,1)));
              }
            if (point == Point(1,2))
              {
                found_triangle = true;
                CPPUNIT_ASSERT((elem->point(0) == Point(0,1) &&
                                elem->point(1) == Point(1,0) &&
                                elem->point(2) == Point(1,2)) ||
                               (elem->point(1) == Point(0,1) &&
                                elem->point(2) == Point(1,0) &&
                                elem->point(0) == Point(1,2)) ||
                               (elem->point(2) == Point(0,1) &&
                                elem->point(0) == Point(1,0) &&
                                elem->point(1) == Point(1,2)));
              }
          }
        CPPUNIT_ASSERT(found_triangle);
      }
  }


  void testTriangle()
  {
#ifdef LIBMESH_HAVE_TRIANGLE
    Mesh mesh(*TestCommWorld);

    TriangleInterface triangle(mesh);

    testTriangulator(mesh, triangle);
#endif
  }


  void testPoly2Tri()
  {
#ifdef LIBMESH_HAVE_POLY2TRI
    Mesh mesh(*TestCommWorld);

    Poly2TriTriangulator p2t_tri(mesh);

    testTriangulator(mesh, p2t_tri);
#endif
  }

};


CPPUNIT_TEST_SUITE_REGISTRATION( MeshTriangulationTest );
