#pragma once

#include <PhysX/PxPhysicsAPI.h>

using namespace physx;

class PhysXManager
{
	protected:
		PhysXManager();
		~PhysXManager();

	public:
		void Init();
		void StepPhysics(float deltaTime);
		void CleanupPhysics();

		void CreateScene();

	public:
		PxDefaultAllocator		gAllocator;
		PxDefaultErrorCallback	gErrorCallback;

		PxFoundation*			gFoundation;
		PxPhysics*				gPhysics;
		PxCooking*				gCooking;

		PxDefaultCpuDispatcher*	gDispatcher;
		PxScene*				gScene;

		PxVisualDebuggerConnection* gConnection;
};