


class LocalSpaceCloth : public Scene
{
public:

	LocalSpaceCloth (const char* name) : Scene(name) {}

	void Initialize()
	{
		float stretchStiffness = 1.0f;
		float bendStiffness = 1.0f;
		float shearStiffness = 1.0f;

		float radius = 0.1f;

		CreateSpringGrid(Vec3(0.5f, 1.45f, -0.5f), 32, 20, 1, radius*0.5f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseSelfCollideFilter), stretchStiffness, bendStiffness, shearStiffness, Vec3(0.0f), 1.0f);
		
		int c1 = 1;
		int c2 = 20;

		// add tethers
		for (int i=0; i < int(g_buffers->positions.size()); ++i)
		{
			float minSqrDist = FLT_MAX;

			if (i != c1 && i != c2)
			{
				float stiffness = -0.8f;
				float give = 0.01f;

				float sqrDist = LengthSq(Vec3(g_buffers->positions[c1])-Vec3(g_buffers->positions[c2]));

				if (sqrDist < minSqrDist)
				{
					CreateSpring(c1, i, stiffness, give);
					CreateSpring(c2, i, stiffness, give);

					minSqrDist = sqrDist;
				}
			}
		}
		


		for (int i=0; i < g_buffers->positions.size(); ++i)
		{
			if (g_buffers->positions[i].x == 0.5f)
				g_buffers->positions[i].w = 0.0f;
		}

		translation = Vec3(0.0f, 1.0f, 0.0f);
		size = Vec3(0.5f, 1.1f, 0.6f);
		rotation = 0.0f;
		rotationSpeed = 0.0f;

		linearInertialScale = 0.25f;
		angularInertialScale = 0.25f;

		AddBox(size, translation);

		// initialize our moving frame to the center of the box
		NvFlexExtMovingFrameInit(&meshFrame, translation, Quat());

		g_numSubsteps = 2;

		g_params.fluid = false;
		g_params.radius = radius;
		g_params.dynamicFriction = 0.f;
		g_params.restitution = 0.0f;
		g_params.shapeCollisionMargin = 0.05f;

		g_params.numIterations = 6;
	
		// draw options		
		g_drawPoints = false;
	}

	virtual void DoGui()
	{
		imguiSlider("Rotation", &rotationSpeed, 0.0f, 20.0f, 0.1f);
		imguiSlider("Translation", &translation.x, -2.0f, 2.0f, 0.001f);

		imguiSlider("Linear Inertia", &linearInertialScale, 0.0f, 1.0f, 0.001f);
		imguiSlider("Angular Inertia", &angularInertialScale, 0.0f, 1.0f, 0.001f);

	}

	virtual void Update()
	{
		rotation += rotationSpeed*g_dt;

		// new position of the box center
		Vec3 newPosition = translation;//meshFrame.GetPosition() + Vec3(float(g_lastdx), 0.0f, float(g_lastdy))*0.001f;
		Quat newRotation = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), rotation);

		NvFlexExtMovingFrameUpdate(&meshFrame, newPosition, newRotation, g_dt);

		// update all the particles in the sim with inertial forces
		NvFlexExtMovingFrameApply(
			&meshFrame, 
			&g_buffers->positions[0].x, 
			&g_buffers->velocities[0].x, 
			g_buffers->positions.size(),
			linearInertialScale,
			angularInertialScale,
			g_dt);

		// update collision shapes
		g_buffers->shapePositions.resize(0);
		g_buffers->shapeRotations.resize(0);
		g_buffers->shapePrevPositions.resize(0);
		g_buffers->shapePrevRotations.resize(0);
		g_buffers->shapeGeometry.resize(0);
		g_buffers->shapeFlags.resize(0);

		AddBox(size, newPosition, newRotation);
		
		UpdateShapes();
	}

	Vec3 size;
	Vec3 translation;
	float rotation;
	float rotationSpeed;

	float linearInertialScale;
	float angularInertialScale;

	NvFlexExtMovingFrame meshFrame;

	NvFlexTriangleMeshId mesh;
};
