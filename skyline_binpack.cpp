/** @file SkylineBinPack.cpp
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the SKYLINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#include "skyline_binpack.h"
#include <algorithm>
#include <limits>
#include <cassert>

namespace binpack {

SkylineBinPack::SkylineBinPack()
: binWidth(0), binHeight(0), usedSurfaceArea(0)
{
}

SkylineBinPack::SkylineBinPack(int width, int height)
: binWidth(0), binHeight(0), usedSurfaceArea(0)
{
	Init(width, height);
}

void SkylineBinPack::Init(int width, int height)
{
	assert(width > 0);
	assert(height > 0);

	binWidth = width;
	binHeight = height;

	usedSurfaceArea = 0;
	skyLine.clear();
	SkylineNode node;
	node.x = 0;
	node.y = 0;
	node.width = binWidth;
	skyLine.push_back(node);
}

Rect SkylineBinPack::Insert(int width, int height)
{
	return InsertBottomLeft(width, height);
}

float SkylineBinPack::Occupancy() const
{
	return (float)usedSurfaceArea / (binWidth * binHeight);
}

Rect SkylineBinPack::InsertBottomLeft(int width, int height)
{
	int bestHeight;
	int bestWidth;
	int bestIndex;
	Rect newNode = FindPositionForNewNodeBottomLeft(width, height, bestHeight, bestWidth, bestIndex);

	if (bestIndex != -1)
	{
		// Perform the actual packing.
		AddSkylineLevel(bestIndex, newNode);

		usedSurfaceArea += width * height;
	}

	return newNode;
}

Rect SkylineBinPack::FindPositionForNewNodeBottomLeft(int width, int height, int &bestHeight, int &bestWidth, int &bestIndex) const
{
	Rect newNode;
	memset(&newNode, 0, sizeof(newNode));
	bestIndex = -1;
	bestHeight = std::numeric_limits<int>::max();
	// Used to break ties if there are nodes at the same level. Then pick the narrowest one.
	bestWidth = std::numeric_limits<int>::max();
	for (size_t i = 0; i < skyLine.size(); ++i)
	{
		int y;
		if (RectangleFits(i, width, height, y))
		{
			if (y + height < bestHeight || (y + height == bestHeight && skyLine[i].width < bestWidth))
			{
				bestHeight = y + height;
				bestIndex = i;
				bestWidth = skyLine[i].width;
				newNode.x = skyLine[i].x;
				newNode.y = y;
				newNode.width = width;
				newNode.height = height;
			}
		}
		//if (RectangleFits(i, height, width, y))
		//{
		//	if (y + width < bestHeight || (y + width == bestHeight && skyLine[i].width < bestWidth))
		//	{
		//		bestHeight = y + width;
		//		bestIndex = i;
		//		bestWidth = skyLine[i].width;
		//		newNode.x = skyLine[i].x;
		//		newNode.y = y;
		//		newNode.width = height;
		//		newNode.height = width;
		//	}
		//}
	}

	return newNode;
}

bool SkylineBinPack::RectangleFits(int skylineNodeIndex, int width, int height, int &y) const
{
	int x = skyLine[skylineNodeIndex].x;
	if (x + width > binWidth)
		return false;
	int widthLeft = width;
	int i = skylineNodeIndex;
	y = skyLine[skylineNodeIndex].y;
	while (widthLeft > 0)
	{
		y = std::max(y, skyLine[i].y);
		if (y + height > binHeight)
			return false;
		widthLeft -= skyLine[i].width;
		++i;
		assert(i < (int)skyLine.size() || widthLeft <= 0);
	}
	return true;
}

void SkylineBinPack::AddSkylineLevel(int skylineNodeIndex, const Rect &rect)
{
	SkylineNode newNode;
	newNode.x = rect.x;
	newNode.y = rect.y + rect.height;
	newNode.width = rect.width;
	skyLine.insert(skyLine.begin() + skylineNodeIndex, newNode);

	assert(newNode.x + newNode.width <= binWidth);
	assert(newNode.y <= binHeight);

	for (size_t i = skylineNodeIndex+1; i < skyLine.size(); ++i)
	{
		assert(skyLine[i-1].x <= skyLine[i].x);

		if (skyLine[i].x < skyLine[i-1].x + skyLine[i-1].width)
		{
			int shrink = skyLine[i-1].x + skyLine[i-1].width - skyLine[i].x;

			skyLine[i].x += shrink;
			skyLine[i].width -= shrink;

			if (skyLine[i].width <= 0)
			{
				skyLine.erase(skyLine.begin() + i);
				--i;
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
	MergeSkylines();
}

void SkylineBinPack::MergeSkylines()
{
	for (size_t i = 0; i < skyLine.size()-1; ++i)
	{
		if (skyLine[i].y == skyLine[i+1].y)
		{
			skyLine[i].width += skyLine[i+1].width;
			skyLine.erase(skyLine.begin() + (i+1));
			--i;
		}
	}
}

}  // End of namespace binpack
