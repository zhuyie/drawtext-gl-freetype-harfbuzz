/** @file SkylineBinPack.h
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <vector>

namespace binpack {

struct Rect
{
	int x;
	int y;
	int width;
	int height;
};

/** Implements bin packing algorithms that use the SKYLINE data structure to store the bin contents. */
class SkylineBinPack
{
public:
	/// Instantiates a bin of size (0,0). Call Init to create a new bin.
	SkylineBinPack();

	/// Instantiates a bin of the given size.
	SkylineBinPack(int binWidth, int binHeight);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int binWidth, int binHeight);

	/// Inserts a single rectangle into the bin.
	Rect Insert(int width, int height);

	/// Computes the ratio of used surface area to the total bin area.
	float Occupancy() const;

private:
	int binWidth;
	int binHeight;

	/// Represents a single level (a horizontal line) of the skyline/horizon/envelope.
	struct SkylineNode
	{
		/// The starting x-coordinate (leftmost).
		int x;

		/// The y-coordinate of the skyline level line.
		int y;

		/// The line width. The ending coordinate (inclusive) will be x+width-1.
		int width;
	};

	std::vector<SkylineNode> skyLine;

	unsigned long usedSurfaceArea;

	Rect InsertBottomLeft(int width, int height);

	Rect FindPositionForNewNodeBottomLeft(int width, int height, int &bestHeight, int &bestWidth, int &bestIndex) const;

	bool RectangleFits(int skylineNodeIndex, int width, int height, int &y) const;

	void AddSkylineLevel(int skylineNodeIndex, const Rect &rect);

	/// Merges all skyline nodes that are at the same level.
	void MergeSkylines();
};

}  // End of namespace binpack
