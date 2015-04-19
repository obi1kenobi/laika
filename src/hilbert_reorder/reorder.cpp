#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cfloat>
#include <string>
#include <algorithm>
#include <iostream>
#include "./common.h"
#include "./io.h"
#include "./libhilbert/hilbert.h"

using namespace std;

#ifndef HILBERTBITS
#define HILBERTBITS 4
#endif

// This function populates the hilbertId field of each vertex_t in nodes, by
// remapping every vertex onto an n^3 lattice using appropriate scaling factors,
// and then traversing the lattice using a 3-D Hilbert curve.
//
// hilbertBits = number of bits in Hilbert grid (grid with side 2^hilbertBits)
void assignHilbertIds(vertex_t * const nodes, const int cntNodes,
                      const unsigned hilbertBits) {
  // We first traverse all vertices to find the maximal and minimal coordinates
  // along each axis: xMin, xMax, yMin, yMax, zMin, zMax.

  double xMin, xMax, yMin, yMax, zMin, zMax;
  uint64_t hilbertGridN = 1 << hilbertBits;

  xMin = yMin = zMin = DBL_MAX;
  xMax = yMax = zMax = DBL_MIN;
  for (int i = 0; i < cntNodes; ++i) {
    xMin = fmin(xMin, nodes[i].x);
    yMin = fmin(yMin, nodes[i].y);
    zMin = fmin(zMin, nodes[i].z);
    xMax = fmax(xMax, nodes[i].x);
    yMax = fmax(yMax, nodes[i].y);
    zMax = fmax(zMax, nodes[i].z);
  }

  WHEN_DEBUG({
    printf("Coordinate extremes:\n");
    printf("xMin -- xMax: %.3f -- %.3f\n", xMin, xMax);
    printf("yMin -- yMax: %.3f -- %.3f\n", yMin, yMax);
    printf("zMin -- zMax: %.3f -- %.3f\n", zMin, zMax);
    printf("\n");
  })

  // We now create a mapping that maps:
  //   (xMin, yMin, zMin) to lattice point (0, 0, 0)
  //   (xMax, yMax, zMax) to lattice point (n-1, n-1, n-1)
  // where n = hilbertGridN
  //
  // This mapping is the following: for the T axis,
  // tLattice = round((t - tMin) * (hilbertGridN - 1) / tMax);

  for (int i = 0; i < cntNodes; ++i) {
    bitmask_t latticeCoords[3];
    bitmask_t hilbertIndex;

    nodes[i].id = (vid_t)i;

    latticeCoords[0] = (uint64_t) round((nodes[i].x - xMin) * (hilbertGridN - 1) / xMax);
    latticeCoords[1] = (uint64_t) round((nodes[i].y - yMin) * (hilbertGridN - 1) / yMax);
    latticeCoords[2] = (uint64_t) round((nodes[i].z - zMin) * (hilbertGridN - 1) / zMax);

    hilbertIndex = hilbert_c2i(3, hilbertBits, latticeCoords);
    nodes[i].hilbertId = (vid_t) hilbertIndex;

    WHEN_DEBUG({
      double coords[3];
      coords[0] = (nodes[i].x - xMin) * (hilbertGridN - 1) / xMax;
      coords[1] = (nodes[i].y - yMin) * (hilbertGridN - 1) / yMax;
      coords[2] = (nodes[i].z - zMin) * (hilbertGridN - 1) / zMax;

      printf("hilbert data for node id %lu:\n", nodes[i].id);
      printf("  rescale X: %.3f\n", coords[0]);
      printf("  rescale Y: %.3f\n", coords[1]);
      printf("  rescale Z: %.3f\n", coords[2]);
      printf("  lattice X: %lld\n", latticeCoords[0]);
      printf("  lattice Y: %lld\n", latticeCoords[1]);
      printf("  lattice Z: %lld\n", latticeCoords[2]);
      printf("  hilbertId: %lld\n", hilbertIndex);
      printf("\n");
    })
  }
}

bool vertexComparator(const vertex_t& a, const vertex_t& b) {
  if (a.hilbertId != b.hilbertId) {
    return a.hilbertId < b.hilbertId;
  } else {
    return a.id < b.id;
  }
}

vid_t * createIdTranslationMapping(vertex_t reorderedNodes, int cntNodes) {
  vid_t * mapping = new (std::nothrow) vid_t[cntNodes];
  assert(mapping != 0);

  for (int i = 0; i < cntNodes; ++i) {
    mapping[reorderedNodes[i].id] = i;
  }

  return mapping;
}

int main(int argc, char *argv[]) {
  vertex_t * nodes;
  int cntNodes;
  char * inputNodeFile, * inputEdgeFile;
  char * outputNodeFile, * outputEdgeFile;

  if (argc != 5) {
    cerr << "\nERROR: Expected 4 arguments, received " << argc << '\n';
    cerr << "Usage: ./reorder <input_nodes> <input_edges> "
            "<output_nodes> <output_edges>" << endl;
    return 1;
  }

  inputNodeFile = argv[1];
  inputEdgeFile = argv[2];
  outputNodeFile = argv[3];
  outputEdgeFile = argv[4];

  cout << "Input node file: " << inputNodeFile << '\n';
  cout << "Input edge file: " << inputEdgeFile << '\n';
  cout << "Output node file: " << outputNodeFile << '\n';
  cout << "Output edge file: " << outputEdgeFile << '\n';
  cout << "Hilbert bits per dimension: " << HILBERTBITS << '\n';

  int result = readNodesFromFile(inputNodeFile, &nodes, &cntNodes);
  assert(result == 0);

  result = readEdgesFromFile(inputEdgeFile, nodes, cntNodes);
  assert(result == 0);

  assignHilbertIds(nodes, cntNodes, HILBERTBITS);

  stable_sort(nodes, nodes + cntNodes, vertexComparator);



  WHEN_DEBUG({
    printf("\nOrder after sorting:\n");
    for (int i = 0; i < cntNodes; ++i) {
      printf("Position %8d: id %8lu\n", i, nodes[i].id);
    }
  })

  return 0;
}
