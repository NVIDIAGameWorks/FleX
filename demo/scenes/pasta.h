
class Pasta : public Scene
{
public:

	Pasta(const char* name) : Scene(name) {}

	virtual void Initialize()
	{
		float radius = 0.1f;
		float length = 15.0f;
		int n = 20;

		for (int i = 0; i < n; ++i)
		{
			float theta = k2Pi*float(i) / n;

			Rope r;
			CreateRope(r, 0.5f*Vec3(cosf(theta), 2.0f, sinf(theta)), Vec3(0.0f, 1.0f, 0.0f), 0.25f, int(length / radius), length, NvFlexMakePhase(0, eNvFlexPhaseSelfCollide));
			g_ropes.push_back(r);
		}

		g_numSubsteps = 3;

		Mesh* bowl = ImportMesh(GetFilePathByPlatform("../../data/bowl.obj").c_str());
		bowl->Normalize(2.0f);
		bowl->CalculateNormals();
		bowl->Transform(TranslationMatrix(Point3(-1.0f, 0.0f, -1.0f)));

		NvFlexTriangleMeshId mesh = CreateTriangleMesh(bowl);
		AddTriangleMesh(mesh, Vec3(), Quat(), 1.0f);

		delete bowl;

		g_params.numIterations = 6;
		g_params.radius = radius;
		g_params.dynamicFriction = 0.4f;
		g_params.dissipation = 0.001f;
		g_params.sleepThreshold = g_params.radius*0.2f;
		g_params.relaxationFactor = 1.3f;
		g_params.restitution = 0.0f;
		g_params.shapeCollisionMargin = 0.01f;

		g_lightDistance *= 0.5f;
		g_drawPoints = false;
	}
};