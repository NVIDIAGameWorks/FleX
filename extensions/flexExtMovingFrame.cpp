#include "../include/NvFlexExt.h"

#include "../core/maths.h"

namespace 
{

// internal version of the public NvFlexExtMovingFrame type
// this must be a byte exact copy of of NvFlexExtMovingFrame
// we just separate the internal types to avoid exposing 
// our math types on the API

struct MovingFrame
{
	Vec3 position;	
	Quat rotation;

	Vec3 velocity;
	Vec3 omega;

	Vec3 acceleration;
	Vec3 tau;

	Matrix44 delta;

	MovingFrame(Vec3 worldTranslation, Quat worldRotation)
	{
		position = worldTranslation;
		rotation = worldRotation;
		delta = Matrix44::kIdentity;
	}
	
	// update the frame, returns a matrix representing the delta transform that 
	// can be applied to particles to teleport them to the frame's new location
	void Move(Vec3 newPosition, Quat newRotation, float dt)
	{
		const float invDt = 1.0f/dt;

		// calculate new velocity
		Vec3 newVelocity = (newPosition-position)*invDt;
		Vec3 newOmega = 2.0f*Vec3(Quat(newRotation-rotation)*Inverse(rotation)*invDt);

		// calculate new acceleration
		Vec3 newAcceleration = (newVelocity-velocity)*invDt;
		Vec3 newTau = (newOmega-omega)*invDt;

		// calculate delta transform
		Matrix44 LocalFromOld = AffineInverse(TranslationMatrix(Point3(position))*RotationMatrix(rotation));
		Matrix44 NewFromLocal = TranslationMatrix(Point3(newPosition))*RotationMatrix(newRotation);
		
		// update delta transform
		delta = NewFromLocal*LocalFromOld;

		// position
		position = newPosition;
		rotation = newRotation;

		// update velocity
		velocity = newVelocity;
		omega = newOmega;

		acceleration = newAcceleration;
		tau = newTau;
	}

	inline Vec3 GetLinearForce() const
	{
		return -acceleration;
	}

	inline Vec3 GetAngularForce(Vec3 p) const
	{
		Vec3 d = p-position;

		// forces due to rotation
		Vec3 centrifugalForce = -Cross(omega, Cross(omega, d));
		Vec3 eulerForce = -Cross(tau, d);

		return centrifugalForce + eulerForce;
	}

};

static_assert(sizeof(NvFlexExtMovingFrame) == sizeof(MovingFrame), "Size mismatch for NvFlexExtMovingFrame");

} // anonymous namespace

void NvFlexExtMovingFrameInit(NvFlexExtMovingFrame* frame, const float* worldTranslation, const float* worldRotation)
{
	((MovingFrame&)(*frame)) = MovingFrame(Vec3(worldTranslation), Quat(worldRotation));
}

void NvFlexExtMovingFrameUpdate(NvFlexExtMovingFrame* frame, const float* worldTranslation, const float* worldRotation, float dt)
{
	((MovingFrame&)(*frame)).Move(Vec3(worldTranslation), Quat(worldRotation), dt);
}

void NvFlexExtMovingFrameApply(NvFlexExtMovingFrame* frame, float* positions, float* velocities, int numParticles, float linearScale, float angularScale, float dt)
{
	const MovingFrame& f = (MovingFrame&)(*frame);

	// linear force constant for all particles
	Vec3 linearForce = f.GetLinearForce()*linearScale;

	for (int i=0; i < numParticles; ++i)
	{
		Vec3 particlePos = Vec3(&positions[i*4]);
		Vec3 particleVel = Vec3(&velocities[i*3]);

		// angular force depends on particles position
		Vec3 angularForce = f.GetAngularForce(particlePos)*angularScale;

		// transform particle to frame's new location
		particlePos = Vec3(f.delta*Vec4(particlePos, 1.0f));
		particleVel += (linearForce + angularForce)*dt;

		// update positions
		positions[i*4+0] = particlePos.x;
		positions[i*4+1] = particlePos.y;
		positions[i*4+2] = particlePos.z;

		velocities[i*3+0] = particleVel.x;
		velocities[i*3+1] = particleVel.y;
		velocities[i*3+2] = particleVel.z;
		
	}
}
