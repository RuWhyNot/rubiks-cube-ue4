#pragma once

#include "Base/Matrix.h"
#include "RCRotationAxis.h"
#include "RubikPart.h"
#include "Engine.h"

namespace RC
{
	class CubeParts
	{
	public:
		CubeParts(int length, int width, int height, float initialBlockSize, FVector centerShift);

		void InsertPart(ARubikPart* part, Coord pos);
		void RotateSlice(RotationAxis axis, int pos, float angle);
		void RenewPartsLocations(RotationAxis axis, int pos);

		inline void SetMainLocation(const FVector& location) { MainLocation = location; };
		inline void SetMainRotation(const FQuat& rotation) { MainRotation = rotation; };

		void UpdateAllParts();

		bool IsAssembled();

		template<typename Functor>
		void ForEachPart(Functor functor)
		{
			for (unsigned int z = 0, zSize = Parts.getHeight(); z < zSize; ++z)
			{
				for (unsigned int y = 0, ySize = Parts.getWidth(); y < ySize; ++y)
				{
					for (unsigned int x = 0, xSize = Parts.getLength(); x < xSize; ++x)
					{
						functor(Parts[x][y][z].ptr);
					}
				}
			}
		}

	private:
		typedef ARubikPart* PartPtr;

		struct PartInfo
		{
			PartPtr ptr{ nullptr };
			FQuat baseRotation{ EForceInit::ForceInitToZero };
			FVector baseLocation{ FVector::ZeroVector };

			FQuat localRotation{ EForceInit::ForceInitToZero };
			FVector localLocation{ FVector::ZeroVector };

			Coord originalCoord;
		};

		typedef Matrix<PartInfo*> Slice;

	private:
		void RotatePart(PartInfo* part, const FRotator& rotation);
		Slice GetSlice(RotationAxis axis, int pos);
		FRotator GetPartInitialRotation(const Coord& coord);
		void RotateSlice(Slice& slice, RotationAxis axis, float angle);
		void MakeSnapshot(Slice& slice);

		void RearrangeControls();
		void RearrangePartControls(AActor * side, const Coord& coord);
		void RearrangeSideControls(AActor * side, const Coord& coord);

	private:
		Array3D<PartInfo> Parts;
		float InitialBlockSize = 0.0f;
		FVector CenterShift;

		FVector MainLocation{ FVector::ZeroVector };
		FQuat MainRotation{ EForceInit::ForceInitToZero };
	};
} // namespace RC
