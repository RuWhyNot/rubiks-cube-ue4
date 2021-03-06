// All right! Yeah

#include "Rubik.h"
#include "RubiksPlayerController.h"
#include "RubicsCube.h"
#include "RubiksSide_Standart.h"
#include "RubikPart.h"

static const float MOVEMENT_SWIPE_TOLERANCE = 10.0f;

bool ARubiksPlayerController::InputKey(FKey Key, EInputEvent EventType, float AmountDepressed, bool bGamepad)
{
	if (Key == FKey("LeftMouseButton") && EventType == EInputEvent::IE_Pressed)
	{
		FVector2D mousePos;
		GetMousePosition(mousePos.X, mousePos.Y);
		SwipeStartActor = GetActorUnderPoint(mousePos);
		StartRotateCubePart(SwipeStartActor, mousePos);
	}
	else if (Key == FKey("LeftMouseButton") && EventType == EInputEvent::IE_Released)
	{
		if (SwipeStartActor != nullptr)
		{
			ProcessTap(SwipeStartActor);
			SwipeStartActor = nullptr;
		}
	}

	Super::InputKey(Key, EventType, AmountDepressed, bGamepad);

	return true;
}

bool ARubiksPlayerController::InputTouch(uint32 Handle, ETouchType::Type Type, const FVector2D& TouchLocation, float Force, FDateTime DeviceTimestamp, uint32 TouchpadIndex)
{
	ThisTouchesMax = 0; // optimization

	if (RotationsLockIndex != -1 && Handle != RotationsLockIndex)
	{
		if (Type == ETouchType::Ended)
		{
			Multitouch.RemoveTouch(Handle);
		}

		return true;
	}

	if (Type == ETouchType::Began)
	{
		AActor* actorUnderPoint = GetActorUnderPoint(TouchLocation);
		if (ActorIsCube(actorUnderPoint))
		{
			RotationsLockIndex = Handle;
			StartRotateCubePart(actorUnderPoint, TouchLocation);
			return true;
		}

		SwipeStartActor = actorUnderPoint;

		Multitouch.AddTouch(Handle, TouchLocation);
		if (ThisTouchesMax < Handle) { ThisTouchesMax = Handle; } // optimization
	}
	else if (Type == ETouchType::Moved)
	{
		if (RotationsLockIndex == Handle)
		{
			TryToRotateCubePart(TouchLocation);
		}

		if (SwipeStartActor != nullptr)
		{
			AActor* actorUnderPoint = GetActorUnderPoint(TouchLocation);
			if (actorUnderPoint != SwipeStartActor)
			{
				SwipeStartActor = nullptr;
			}
		}

		Multitouch.MoveTouch(Handle, TouchLocation);
		if (ThisTouchesMax < Handle) { ThisTouchesMax = Handle; } // optimization
	}
	else if (Type == ETouchType::Ended)
	{
		Multitouch.RemoveTouch(Handle);

		if (LastTouchesMax == Handle) { LastTouchesMax = ThisTouchesMax; } // optimization

		if (Handle == RotationsLockIndex)
		{
			RotationsLockIndex = -1;
		}

		if (SwipeStartActor != nullptr)
		{
			ProcessTap(SwipeStartActor);
			SwipeStartActor = nullptr;
		}
	}

	if (LastTouchesMax < ThisTouchesMax) { LastTouchesMax = ThisTouchesMax; } // optimization

	if (Handle < LastTouchesMax) // optimization
	{
		return true;
	}

	if (Multitouch.GetTouchesCount() == 1)
	{
		FVector2D movement = Multitouch.GetMoveVector() * TouchRotationSensitivity;
		RotateCube(FRotator(movement.Y, -movement.X, 0.0f));
	}
	else
	{
		float spin = Multitouch.GetSpinAngle() * TwoFingersRotationSensitivity;
		RotateCube(FRotator(0.0f, 0.0f, -spin));
	}

	return true;
}

bool ARubiksPlayerController::InputAxis(FKey Key, float Delta, float DeltaTime, int32 NumSamples, bool bGamepad)
{
	if (IsInputKeyDown(FKey("RightMouseButton")))
	{
		FVector2D mouseDelta;
		GetInputMouseDelta(mouseDelta.X, mouseDelta.Y);

		if (IsInputKeyDown(FKey("LeftShift")))
		{
			RotateCube(FRotator(0.0f, 0.0f, mouseDelta.X * RMBSensitivity));
		}
		else
		{
			RotateCube(FRotator(-mouseDelta.Y * RMBSensitivity, -mouseDelta.X * RMBSensitivity, 0.0f));
		}
	}
	else if (IsInputKeyDown(FKey("LeftMouseButton")))
	{
		FVector2D mousePos;
		GetMousePosition(mousePos.X, mousePos.Y);
		TryToRotateCubePart(mousePos);

		if (SwipeStartActor != nullptr)
		{
			AActor* actorUnderPoint = GetActorUnderPoint(mousePos);
			if (actorUnderPoint != SwipeStartActor)
			{
				SwipeStartActor = nullptr;
			}
		}
	}

	Super::InputAxis(Key, Delta, DeltaTime, NumSamples, bGamepad);
	return true;
}

bool ARubiksPlayerController::InputMotion(const FVector& Tilt, const FVector& RotationRate, const FVector& Gravity, const FVector& Acceleration)
{
	return true;
}

ARubicsCube * ARubiksPlayerController::GetCube()
{
	return MainCube;
}

void ARubiksPlayerController::SetCube(ARubicsCube* NewCube)
{
	MainCube = NewCube;
}

void ARubiksPlayerController::SetRMBSensitivity(float NewSensitivity)
{
	RMBSensitivity = NewSensitivity;
}

void ARubiksPlayerController::SetTouchRotationSensitivity(float NewSensitivity)
{
	TouchRotationSensitivity = NewSensitivity;
}

void ARubiksPlayerController::SetTwoFingersRotationSensitivity(float NewSensitivity)
{
	TwoFingersRotationSensitivity = NewSensitivity;
}

void ARubiksPlayerController::RotateCube(const FRotator& rotation)
{
	if (IsCubeReady())
	{
		FQuat fullRotation = FQuat(rotation) * FQuat(MainCube->GetActorRotation());
		FQuat partialRotation = FQuat::FastLerp(MainCube->GetActorRotation().Quaternion(), fullRotation, 0.3);
		MainCube->SetActorRotation(partialRotation.Rotator());
		MainCube->ScheduleUpdateParts();
	}
}

bool ARubiksPlayerController::IsCubeReady() const
{
	return MainCube && MainCube->IsValidLowLevel();
}

bool ARubiksPlayerController::ActorIsCube(const AActor* Actor) const
{
	if (Actor && Actor->IsValidLowLevel() && Actor->IsA(ARubikPart::StaticClass()))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ARubiksPlayerController::StartRotateCubePart(AActor* CubePart, const FVector2D& TouchLocation)
{
	if (CubePart && CubePart->IsValidLowLevel() && CubePart->IsA(ARubiksSide_Standart::StaticClass()))
	{
		FVector2D ActorScreenPos;
		ProjectWorldLocationToScreen(CubePart->GetActorLocation(), ActorScreenPos);

		if (IsCubeReady())
		{
			CurrentSideDirections.Empty();
			TArray<RC::MovementDirection> Directions = dynamic_cast<ARubiksSide_Standart*>(CubePart)->GetDirections();
			for (const auto& Direction : Directions)
			{
				FVector2D ScreenLocation;
				ProjectWorldLocationToScreen(CubePart->GetActorLocation() + FQuat(MainCube->GetActorRotation()).RotateVector(Direction.direction), ScreenLocation);
				CurrentSideDirections.Push(RC::ScreenMovementDiraction(ScreenLocation - ActorScreenPos, Direction.axis, Direction.layerIndex));
			}
			RotationCompleted = false;
		}

		MovementTouchStartLocation = TouchLocation;
	}
}

void ARubiksPlayerController::TryToRotateCubePart(const FVector2D& TouchLocation)
{
	if (IsCubeReady() && !RotationCompleted)
	{
		float minimalAngle = 1000.0f;
		RC::ScreenMovementDiraction bestDirection;
		FVector2D movementDelta = TouchLocation - MovementTouchStartLocation;
		float movementLength = -1.0f;

		for (const auto& direction : CurrentSideDirections)
		{
			float angle = FMath::Atan2(direction.direction.X, direction.direction.Y) - FMath::Atan2(movementDelta.X, movementDelta.Y);
			if (angle > PI) { angle -= 2 * PI; }
			if (angle < -PI) { angle += 2 * PI; }
			if (FMath::Abs(angle) < minimalAngle)
			{
				minimalAngle = FMath::Abs(angle);
				bestDirection = direction;
				movementLength = direction.direction.Size();
			}
		}

		if (movementLength > 0.0f && movementDelta.Size() > (movementLength * MOVEMENT_SWIPE_TOLERANCE))
		{
			MainCube->AddRotation(bestDirection.axis, bestDirection.layerIndex);
			MainCube->OnMoveDone.Broadcast();
			RotationCompleted = true;
		}
	}
}

AActor* ARubiksPlayerController::GetActorUnderPoint(const FVector2D& point) const
{
	FVector location, direction;
	DeprojectScreenPositionToWorld(point.X, point.Y, location, direction);

	FCollisionQueryParams TraceParams(FName(TEXT("")), true, this);
	TraceParams.bReturnPhysicalMaterial = false;
	TraceParams.bTraceComplex = false;

	FHitResult hit(ForceInit);
	float endOffset = 500;
	FVector target = location + (direction * endOffset);
	GetWorld()->LineTraceSingleByChannel(hit, location, target, ECC_GameTraceChannel1, TraceParams);
	return hit.GetActor();
}
