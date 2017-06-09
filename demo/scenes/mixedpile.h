
/*
class MixedPile : public Scene
{
public:

	MixedPile(const char* name) : Scene(name)
	{
	}
	

	std::vector<ClothMesh*> mCloths;
	std::vector<float> mRestVolume;
	std::vector<int> mTriOffset;
	std::vector<int> mTriCount;
	std::vector<float> mOverPressure;
	std::vector<float> mConstraintScale;
	std::vector<float> mSplitThreshold;

	void AddInflatable(const Mesh* mesh, float overPressure, int phase, float invmass=1.0f)
	{
		const int startVertex = g_buffers->positions.size();

		// add mesh to system
		for (size_t i=0; i < mesh->GetNumVertices(); ++i)
		{
			const Vec3 p = Vec3(mesh->m_positions[i]);
				
			g_buffers->positions.push_back(Vec4(p.x, p.y, p.z, invmass));
			g_buffers->velocities.push_back(0.0f);
			g_buffers->phases.push_back(phase);
		}

		int triOffset = g_buffers->triangles.size();
		int triCount = mesh->GetNumFaces();

		mTriOffset.push_back(triOffset/3);
		mTriCount.push_back(mesh->GetNumFaces());
		mOverPressure.push_back(overPressure);

		for (size_t i=0; i < mesh->m_indices.size(); i+=3)
		{
			int a = mesh->m_indices[i+0];
			int b = mesh->m_indices[i+1];
			int c = mesh->m_indices[i+2];

			Vec3 n = -Normalize(Cross(mesh->m_positions[b]-mesh->m_positions[a], mesh->m_positions[c]-mesh->m_positions[a]));
			g_buffers->triangleNormals.push_back(n);

			g_buffers->triangles.push_back(a + startVertex);
			g_buffers->triangles.push_back(b + startVertex);
			g_buffers->triangles.push_back(c + startVertex);
		}

		// create a cloth mesh using the global positions / indices
		ClothMesh* cloth = new ClothMesh(&g_buffers->positions[0], g_buffers->positions.size(), &g_buffers->triangles[triOffset],triCount*3, 0.8f, 1.0f);

		for (size_t i=0; i < cloth->mConstraintIndices.size(); ++i)
			g_buffers->springIndices.push_back(cloth->mConstraintIndices[i]);

		for (size_t i=0; i < cloth->mConstraintCoefficients.size(); ++i)
			g_buffers->springStiffness.push_back(cloth->mConstraintCoefficients[i]);

		for (size_t i=0; i < cloth->mConstraintRestLengths.size(); ++i)
			g_buffers->springLengths.push_back(cloth->mConstraintRestLengths[i]);

		mCloths.push_back(cloth);

		// add inflatable params
		mRestVolume.push_back(cloth->mRestVolume);
		mConstraintScale.push_back(cloth->mConstraintScale);
	}


	virtual void Initialize()
	{
		
		Vec3 start(0.0f, 0.5f + g_params.radius*0.25f, 0.0f);

		float radius = g_params.radius;

		int group = 1;
		
		if (1)
		{
			mCloths.resize(0);
			mRestVolume.resize(0);
			mTriOffset.resize(0);
			mTriCount.resize(0);
			mOverPressure.resize(0);
			mConstraintScale.resize(0);
			mSplitThreshold.resize(0);

			Vec3 lower(0.0f), upper(0.0f);
			float size = 1.0f + radius;

			for (int i=0; i < 9; ++i)
				{
				Mesh* mesh = ImportMesh(GetFilePathByPlatform("../../data/sphere.ply").c_str());
				mesh->Normalize();
				mesh->Transform(TranslationMatrix(Point3(lower.x + i%3*size, upper.y + 2.0f, (upper.z+lower.z)*0.5f + i/3*size)));
				
				AddInflatable(mesh, 1.0f, NvFlexMakePhase(group++, 0), 2.0f);
				delete mesh;
			}
		}


		if (1)
		{
			const int minSize[3] = { 2, 1, 3 };
			const int maxSize[3] = { 4, 3, 6 };

			Vec4 color = Vec4(SrgbToLinear(Colour(Vec4(201.0f, 158.0f, 106.0f, 255.0f)/255.0f)));

			Vec3 lower(0.0f), upper(5.0f);
			GetParticleBounds(lower,upper);

			int dimx = 3;
			int dimy = 10;
			int dimz = 3;

			for (int y=0; y < dimy; ++y)
			{
				for (int z=0; z < dimz; ++z)
				{
					for (int x=0; x < dimx; ++x)
					{
						CreateParticleShape(
						GetFilePathByPlatform("../../data/box.ply").c_str(), 					
						Vec3(x + 0.5f,0,z+ 0.5f)*(1.0f+radius) + Vec3(0.0f, upper.y + (y+2.0f)*maxSize[1]*g_params.radius, 0.0f),
						Vec3(float(Rand(minSize[0], maxSize[0])),
							 float(Rand(minSize[1], maxSize[1])), 
							 float(Rand(minSize[2], maxSize[2])))*g_params.radius*0.9f, 0.0f, g_params.radius*0.9f, Vec3(0.0f), 1.0f, true, 1.0f, NvFlexMakePhase(group++, 0), true,0.0f,0.0f, 0.0f, color);
					}
				}
			}
		}		

		
		if (1)
		{
			Vec3 lower, upper;
			GetParticleBounds(lower,upper);
			Vec3 center = (upper+lower)*0.5f;
			center.y = upper.y;
		
			for (int i=0; i < 20; ++i)
			{
				Rope r;
				Vec3 offset = Vec3(sinf(k2Pi*float(i)/20), 0.0f, cosf(k2Pi*float(i)/20));

				CreateRope(r, center + offset, Normalize(offset + Vec3(0.0f, 4.0f, 0.0f)), 1.2f, 50, 50*radius, NvFlexMakePhase(group++, eNvFlexPhaseSelfCollide), 0.0f, 10.0f, 0.0f);
				g_ropes.push_back(r);
			}
		}

		Vec3 lower, upper;
		GetParticleBounds(lower, upper);

		Vec3 center = (lower+upper)*0.5f;
		center.y = 0.0f;

		float width = (upper-lower).x;
		float edge = 0.25f;
		float height = 1.0f;
		AddBox(Vec3(edge, height, width), center + Vec3(-width, height/2.0f, 0.0f));
		AddBox(Vec3(edge, height, width), center + Vec3(width, height/2.0f, 0.0f));

		AddBox(Vec3(width-edge, height, edge), center + Vec3(0.0f, height/2.0f, width-edge));
		AddBox(Vec3(width-edge, height, edge), center + Vec3(0.0f, height/2.0f, -(width-edge)));
	
		//g_numExtraParticles = 32*1024;		
		g_numSubsteps = 2;
		g_params.numIterations = 7;

		g_params.radius *= 1.0f;
		g_params.solidRestDistance = g_params.radius;
		g_params.fluidRestDistance = g_params.radius*0.55f;
		g_params.dynamicFriction = 0.6f;
		g_params.staticFriction = 0.75f;
		g_params.particleFriction = 0.3f;
		g_params.dissipation = 0.0f;
		g_params.particleCollisionMargin = g_params.radius*0.05f;
		g_params.sleepThreshold = g_params.radius*0.125f;
		g_params.shockPropagation = 0.0f;
		g_params.restitution = 0.0f;
		g_params.collisionDistance = g_params.radius*0.5f;
		g_params.maxSpeed = 2.0f*g_params.radius*g_numSubsteps/g_dt;

		// separte solid particle count
		g_numSolidParticles = g_buffers->positions.size();
		// number of fluid particles to allocate
		g_numExtraParticles = 32*1024;		
		
		g_params.numPlanes = 1;
		g_windStrength = 0.0f;

		g_lightDistance *= 0.5f;

		// draw options
		g_drawPoints = true;
		g_expandCloth = g_params.radius*0.5f;
		
		g_emitters[0].mEnabled = true;
		g_emitters[0].mSpeed = (g_params.radius*0.5f/g_dt);
		g_emitters[0].mSpeed = (g_params.radius/g_dt);

		extern Colour g_colors[];
		g_colors[0] = Colour(0.805f, 0.702f, 0.401f);		
	}

	virtual void Update()
	{
		NvFlexSetInflatables(g_solver, &mTriOffset[0], &mTriCount[0], &mRestVolume[0], &mOverPressure[0], &mConstraintScale[0], mCloths.size(), eFlexMemoryHost);
	}
	
	int mHeight;
};
*/
