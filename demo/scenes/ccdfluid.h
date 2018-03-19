
class CCDFluid : public Scene
{
public:

	CCDFluid (const char* name) : Scene(name) {}

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

		//Mesh* shape = ImportMesh("../../data/box.ply");
		//shape->Transform(ScaleMatrix(Vec3(2.0f)));

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
		newOffset = g_buffers->shapePositions[0].x;
		newRotation = 0.0f;
		rotationSpeed = 0.0f;

		g_numSubsteps = 2;

		g_params.radius = radius;
		g_params.fluidRestDistance = restDistance;
		g_params.dynamicFriction = 0.1f;
		g_params.restitution = 0.0f;
		g_params.shapeCollisionMargin = 0.05f;
		//g_params.maxAcceleration = 50.0f;
		g_params.maxSpeed = g_numSubsteps*restDistance/g_dt;
		g_params.collisionDistance = restDistance;
		//g_params.shapeCollisionMargin = 0.0001f;

		g_params.numIterations = 3;
		//g_params.relaxationFactor = 0.5f;

		g_params.smoothing = 0.4f;

		g_params.viscosity = 0.001f;
		g_params.cohesion = 0.05f;
		g_params.surfaceTension = 0.0f;
		
		// draw options		
		g_drawPoints = true;
		g_drawEllipsoids = false;
		g_drawDiffuse = true;
	}

	virtual void DoGui() 
	{
		imguiSlider("Linear", &newOffset, 0.0f, 2.0f, 0.0001f);
		imguiSlider("Rotation", &rotationSpeed, 0.0f, 10.0f, 0.1f);
	}

	virtual void Update()
	{
		g_buffers->shapePrevPositions[0].x = g_buffers->shapePositions[0].x;
		g_buffers->shapePositions[0].x = newOffset;
		
		newRotation += rotationSpeed*g_dt;

		g_buffers->shapePrevRotations[0] = g_buffers->shapeRotations[0];
		g_buffers->shapeRotations[0] = QuatFromAxisAngle(Vec3(0.0f, 1.0f, 0.0f), newRotation);

		// update previous transform of the disc
		UpdateShapes();
	}

	float newOffset;
	float newRotation;

	float rotationSpeed;

	NvFlexTriangleMeshId mesh;
};
		