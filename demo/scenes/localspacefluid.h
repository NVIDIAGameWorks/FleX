

class LocalSpaceFluid : public Scene
{
public:

	LocalSpaceFluid (const char* name) : Scene(name) {}

	void Initialize()
	{
		const float radius = 0.05f;
		const float restDistance = radius*0.6f;

		int dx = int(ceilf(1.f / restDistance));
		int dy = int(ceilf(1.f / restDistance));
		int dz = int(ceilf(1.f / restDistance));

		CreateParticleGrid(Vec3(0.0f, 1.0f, 0.0f), dx, dy, dz, restDistance, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide | eNvFlexPhaseFluid), restDistance*0.01f);

		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		Vec3 center = (lower+upper)*0.5f;

		Mesh* shape = ImportMesh("../../data/torus.obj");
		shape->Transform(ScaleMatrix(Vec3(0.7f)));
		
		//Mesh* box = ImportMesh("../../data/sphere.ply");
		//box->Transform(TranslationMatrix(Point3(0.0f, 0.1f, 0.0f))*ScaleMatrix(Vec3(1.5f)));

		// invert box faces
		for (int i=0; i < int(shape->GetNumFaces()); ++i)
			swap(shape->m_indices[i*3+0], shape->m_indices[i*3+1]);		

		shape->CalculateNormals();

		// shift into torus interior
		for (int i=0; i < g_buffers->positions.size(); ++i)
			(Vec3&)(g_buffers->positions[i]) -= Vec3(2.1f, 0.0f, 0.0f);

		mesh = CreateTriangleMesh(shape);
		AddTriangleMesh(mesh, Vec3(center), Quat(), 1.0f);

		// initialize our moving frame to the center of the box
		NvFlexExtMovingFrameInit(&meshFrame, center, Quat());

		g_numSubsteps = 2;

		g_params.fluid = true;
		g_params.radius = radius;
		g_params.fluidRestDistance = restDistance;
		g_params.dynamicFriction = 0.f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = 0.05f;
		g_params.shapeCollisionMargin = 0.00001f;
		g_params.maxSpeed = g_numSubsteps*restDistance/g_dt;		

		g_params.numIterations = 4;

		g_params.smoothing = 0.4f;
		g_params.anisotropyScale = 3.0f / radius;
		g_params.viscosity = 0.001f;
		g_params.cohesion = 0.05f;
		g_params.surfaceTension = 0.0f;
		
		translation = center;
		rotation = 0.0f;
		rotationSpeed = 0.0f;

		linearInertialScale = 0.25f;
		angularInertialScale = 0.75f;

		// draw options		
		g_drawPoints = false;
		g_drawEllipsoids = true;
		g_drawDiffuse = true;
	}

	virtual void DoGui()
	{
		imguiSlider("Rotation", &rotationSpeed, 0.0f, 7.0f, 0.1f);
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

		// update torus transform
		g_buffers->shapePrevPositions[0] = g_buffers->shapePositions[0];
		g_buffers->shapePrevRotations[0] = g_buffers->shapeRotations[0];

		g_buffers->shapePositions[0] = Vec4(newPosition, 1.0f);
		g_buffers->shapeRotations[0] = newRotation;

		UpdateShapes();
	}

	Vec3 translation;
	float rotation;
	float rotationSpeed;

	float linearInertialScale;
	float angularInertialScale;

	NvFlexExtMovingFrame meshFrame;

	NvFlexTriangleMeshId mesh;
};