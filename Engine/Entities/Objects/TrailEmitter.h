#ifndef LEVIATHAN_TRAILEMITTER
#define LEVIATHAN_TRAILEMITTER
// ------------------------------------ //
#ifndef LEVIATHAN_DEFINE
#include "Define.h"
#endif
// ------------------------------------ //
// ---- includes ---- //
#include "..\Bases\BaseObject.h"
#include "..\Bases\BaseParentable.h"
#include "..\Bases\BaseRenderable.h"


namespace Leviathan{ namespace Entity{

	struct TrailElementProperties{

		TrailElementProperties(const Float4 &initialcolour, const Float4 &colourchange, const float &initialsize, const float &sizechange) : 
			InitialColour(initialcolour), ColourChange(colourchange), InitialSize(initialsize), SizeChange(sizechange)
		{

		}
		TrailElementProperties(const Float4 &initialcolour, const float &initialsize) : 
			InitialColour(initialcolour), ColourChange(0), InitialSize(initialsize), SizeChange(0)
		{

		}

		Float4 InitialColour;
		Float4 ColourChange;
		float InitialSize;
		float SizeChange;
	};

	struct TrailProperties{
	public:
		TrailProperties(int maxelements, float lenght, float maxdistance, bool castshadows = false) : ElementProperties(maxelements), 
			TrailLenght(lenght), MaxDistance(maxdistance), CastShadows(castshadows)
		{
		}
		~TrailProperties(){
			// Delete memory //
			SAFE_DELETE_VECTOR(ElementProperties);
		}

		float TrailLenght;
		float MaxDistance;
		int MaxChainElements;
		bool CastShadows;

		std::vector<TrailElementProperties*> ElementProperties;
	};

	class TrailEmitter : virtual public BaseObject, public BaseParentable, public BaseRenderable{
	public:
		DLLEXPORT TrailEmitter(GameWorld* world, bool hidden = false);
		DLLEXPORT virtual ~TrailEmitter();

		// Creates the actual trail object //
		DLLEXPORT bool Init(const string materialname, const TrailProperties &variables);
		DLLEXPORT virtual void Release();

		// Sets properties on the trail object (called by init) //
		DLLEXPORT bool SetTrailProperties(const TrailProperties &variables);

		DLLEXPORT virtual bool SendCustomMessage(int entitycustommessagetype, void* dataptr);

	private:
		// Used to reposition the node //
		virtual void PosUpdated();
		virtual void OrientationUpdated();

		virtual void _OnHiddenStateUpdated();
		// ------------------------------------ //

		// Ogre handles pretty much everything related to trails //
		Ogre::RibbonTrail* TrailEntity;
		// This node controls where the trail appears (as the actual entity is in the root node) //
		Ogre::SceneNode* TrailLocation;
	};

}}
#endif