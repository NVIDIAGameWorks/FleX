
class TriangleCollision : public Scene
{
public:

	TriangleCollision(const char* name) : Scene(name) {}

	void Initialize()
	{
		float radius = 0.05f;
		CreateParticleGrid(Vec3(0.4f, 1.0f + radius*0.5f, 0.1f), 10, 5, 10, radius, Vec3(0.0f), 1.0f, false, 0.0f, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide), 0.0f);

		Mesh* disc = CreateDiscMesh(1.0f, 4);

		// create shallow bowl
		disc->m_positions[0].y -= 0.5f;
		disc->CalculateNormals();


		NvFlexTriangleMeshId mesh1 = CreateTriangleMesh(disc);
		AddTriangleMesh(mesh1, Vec3(0.0f, 0.5f, 0.0f), Quat(), Vec3(1.0f, 0.5f, 1.0f));
		AddTriangleMesh(mesh1, Vec3(1.0f, 0.5f, 1.0f), Quat(), Vec3(1.0f, 0.5f, 1.0f));

		NvFlexTriangleMeshId mesh2 = CreateTriangleMesh(disc);
		AddTriangleMesh(mesh2, Vec3(-1.0f, 0.5f, 1.0f), Quat(), Vec3(1.0f, 0.5f, 1.0f));
		AddTriangleMesh(mesh2, Vec3(1.0f, 0.5f, -1.0f), Quat(), Vec3(1.0f, 1.0f, 1.0f));

		NvFlexTriangleMeshId mesh3 = CreateTriangleMesh(disc);
		AddTriangleMesh(mesh3, Vec3(-1.0f, 0.5f, -1.0f), Quat(), Vec3(1.0f, 0.25f, 1.0f));


		delete disc;

		g_params.radius = radius;
		g_params.dynamicFriction = 0.025f;
		g_params.dissipation = 0.0f;
		g_params.restitution = 0.0;
		g_params.numIterations = 4;
		g_params.particleCollisionMargin = g_params.radius*0.05f;

		g_numSubsteps = 1;

		// draw options		
		g_drawPoints = true;
	}
	
};
