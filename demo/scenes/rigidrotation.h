

class RigidRotation : public Scene
{
public:

	RigidRotation(const char* name) : Scene(name)
	{
	}

	void Initialize()
	{
		float radius = 0.1f;

		float dimx = 1.0f;
		float dimy = 5.0f;
		float dimz = 1.0f;

		CreateParticleShape(GetFilePathByPlatform("../../data/box.ply").c_str(), Vec3(0.0f, 1.0f, 0.0f), Vec3(dimx, dimy, dimz)*radius, 0.0f, radius, Vec3(0.0f), 1.0f, true, 1.0f, 0, true, 0.0f, 0.0f, 0.0f);

		g_params.radius = radius;
		g_params.gravity[1] = 0;

		g_params.numIterations = 1;
		g_numSubsteps = 1;

		g_pause = true;

		g_drawBases = true;
	}

	void Update()
	{
		if (g_frame == 0)
		{
			// rotate particles by 90 degrees
			Vec3 lower, upper;
			GetParticleBounds(lower, upper);

			Vec3 center = (lower + upper)*0.5f;

			Matrix44 rotation = RotationMatrix(DegToRad(95.0f), Vec3(0.0f, 0.0f, 1.0f));

			for (int i = 0; i < int(g_buffers->positions.size()); ++i)
			{
				Vec3 delta = Vec3(g_buffers->positions[i]) - center;

				delta = Vec3(rotation*Vec4(delta, 1.0f));

				g_buffers->positions[i].x = center.x + delta.x;
				g_buffers->positions[i].y = center.y + delta.y;
				g_buffers->positions[i].z = center.z + delta.z;
			}
		}
	}
};