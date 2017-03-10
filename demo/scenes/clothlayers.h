

class ClothLayers : public Scene
{
public:

	ClothLayers(const char* name) :
		Scene(name) {}

	virtual void Initialize()
	{

		float stretchStiffness = 1.0f;
		float bendStiffness = 0.8f;
		float shearStiffness = 0.5f;

		int dimx = 64;
		int dimz = 64;
		float radius = 0.05f;
		int phase = NvFlexMakePhase(0, eNvFlexPhaseSelfCollide);

#if 1
		CreateSpringGrid(Vec3(-0.6f, 2.9f, -0.6f), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);
		CreateSpringGrid(Vec3(-0.6f, 2.6f, -0.6f), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);
		CreateSpringGrid(Vec3(-0.6f, 2.3f, -0.6f), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);
		CreateSpringGrid(Vec3(-0.6f, 2.0f, -0.6f), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);

		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		Mesh* sphere = ImportMesh(GetFilePathByPlatform("../../data/sphere.ply").c_str());
		sphere->Normalize();

		NvFlexTriangleMeshId mesh = CreateTriangleMesh(sphere);
		AddTriangleMesh(mesh, Vec3(), Quat(), 2.0f);

		delete sphere;
#else
		// This scene can cause the cloth to bounce
		// Might need to run it a few times to repro
		CreateSpringGrid(Vec3(-0.6f, 2.9f, -0.6f), dimx, dimz, 1, radius, phase, stretchStiffness, bendStiffness, shearStiffness, 0.0f, 1.0f);

		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		Mesh* disc = CreateDiscMesh(2.0f, 300);
		disc->m_positions[0].y -= 0.25f;
		disc->CalculateNormals();
		NvFlexTriangleMeshId mesh = CreateTriangleMesh(disc);
		AddTriangleMesh(mesh, Vec3(0.0f, 2.88f, 1.0f), Quat(), 1.0f);
		delete disc;

		Mesh* disc1 = CreateDiscMesh(2.0f, 250);
		disc1->m_positions[0].y -= 0.25f;
		disc1->CalculateNormals();
		NvFlexTriangleMeshId mesh1 = CreateTriangleMesh(disc1);
		AddTriangleMesh(mesh1, Vec3(1.0f, 1.5f, 0.0f), Quat(), 1.0f);
		delete disc1;
#endif

		g_params.radius = radius*1.0f;
		g_params.dynamicFriction = 0.1625f;
		g_params.dissipation = 0.0f;
		g_params.numIterations = 8;
		g_params.viscosity = 0.0f;
		g_params.drag = 0.05f;
		g_params.collisionDistance = radius;
		g_params.shapeCollisionMargin = radius*0.1f;
		g_params.relaxationFactor = 1.3f;

		g_numSubsteps = 3;


		g_windStrength = 0.0f;

		// draw options
		g_drawPoints = true;
		g_drawSprings = false;
	}
};

