
class Darts : public Scene
{
public:

	Darts(const char* name) : Scene(name) {}

	void Initialize()
	{
		float radius = 0.1f;
		int phase = NvFlexMakePhase(0, 0);

		if (1)
		{
			Vec3 v = Vec3(10.0f, 0.0f, 0.0f);

			float y = 8.0f;

			g_buffers->positions.push_back(Vec4(0.0f, y, 0.0f, 1.0f));
			g_buffers->velocities.push_back(v);
			g_buffers->phases.push_back(phase);

			g_buffers->positions.push_back(Vec4(-1.0f, y, -0.5f, 0.9f));
			g_buffers->velocities.push_back(v);
			g_buffers->phases.push_back(phase);

			g_buffers->positions.push_back(Vec4(-1.0f, y, 0.5f, 0.9f));
			g_buffers->velocities.push_back(v);
			g_buffers->phases.push_back(phase);

			g_buffers->triangles.push_back(0);
			g_buffers->triangles.push_back(1);
			g_buffers->triangles.push_back(2);
			g_buffers->triangleNormals.push_back(0.0f);

			CreateSpring(0, 1, 1.0f);
			CreateSpring(1, 2, 1.0f);
			CreateSpring(2, 0, 1.0f);

			g_buffers->positions[0].y -= radius*2.5f;
			//g_buffers->positions[0].x = 1.0f;
			//g_buffers->positions[1].y += radius;
		}

		g_params.drag = 0.01f;
		g_params.lift = 0.1f;
		g_params.dynamicFriction = 0.25f;
		//g_params.gravity[1] = 0.0f;

		g_drawPoints = false;
	}

	void Update()
	{
		g_params.wind[0] = 0.0f;
		g_params.wind[1] = 0.0f;
		g_params.wind[2] = 0.0f;
	}
};
