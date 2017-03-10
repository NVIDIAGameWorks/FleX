

class Deformables : public Scene
{
public:

	Deformables(const char* name) : Scene(name) {}

	void Initialize()
	{
		g_params.dynamicFriction = 0.25f;

		for (int i=0; i < 5; i++)
			AddRandomConvex(10, Vec3(i*2.0f, 0.0f, Randf(0.0f, 2.0f)), 0.5f, 1.0f, Vec3(0.0f, 1.0f, 0.0f), Randf(0.0f, k2Pi));

		if (0)
		{
			int group = 0;

			float minSize = 0.2f;
			float maxSize = 0.4f;

			for (int z=0; z < 1; ++z)
				for (int y=0; y < 1; ++y)
					for (int x=0; x < 5; ++x)
						CreateRandomBody(12, Vec3(2.0f*x, 2.0f + y, 1.0f + 1.0f*z), minSize, maxSize, RandomUnitVector(), Randf(0.0f, k2Pi), 1.0f, NvFlexMakePhase(group++, 0), 0.25f);
		}
		else
		{
			CreateTetMesh(GetFilePathByPlatform("../../data/tets/duck.tet").c_str(), Vec3(2.0f, 1.0f, 2.0f), 2.00000105f, 1.0f, 0);
			CreateTetMesh(GetFilePathByPlatform("../../data/tets/duck.tet").c_str(), Vec3(2.0f, 3.0f, 2.0f), 2.00000105f, 1.0f, 1);
		}
	
		g_params.numIterations = 5;
		g_params.relaxationFactor = 1.0f;
		g_params.radius = 0.025f;

		// draw options		
		g_drawPoints = true;
		g_drawSprings = false;
	}
};
