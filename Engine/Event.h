#ifndef LEVIATHAN_EVENT
#define LEVIATHAN_EVENT
// ------------------------------------ //
#ifndef LEVIATHAN_DEFINE
#include "Define.h"
#endif
// ------------------------------------ //
// ---- includes ---- //
																// NOTE: when GUI sends key presses or downs it expects receiver to modify GKey to match the actual special keys that were needed to process the event
enum EVENT_TYPE{ EVENT_TYPE_ERROR = 0, EVENT_TYPE_WAKEUP, EVENT_TYPE_GENERAL , EVENT_TYPE_KEYPRESS, EVENT_TYPE_KEYDOWN, 
	EVENT_TYPE_SHOW, EVENT_TYPE_HIDE, EVENT_TYPE_ONCLICK, EVENT_TYPE_BECOMEFOREGROUND, EVENT_TYPE_BECOMEBACKGROUND, EVENT_TYPE_TICK, 
	EVENT_TYPE_ANIMATION_FINISH, EVENT_TYPE_REMOVE, EVENT_TYPE_EVENT_SEQUENCE_BEGIN, EVENT_TYPE_EVENT_SEQUENCE_END,
	EVENT_TYPE_MOUSEMOVED, EVENT_TYPE_MOUSEPOSITION,
	EVENT_TYPE_GUIDISABLE, EVENT_TYPE_GUIENABLE, EVENT_TYPE_WINDOW_RESIZE, EVENT_TYPE_RESIZE, EVENT_TYPE_TEST,
	EVENT_TYPE_FRAME_BEGIN, EVENT_TYPE_FRAME_END, EVENT_TYPE_ENGINE_TICK,
	EVENT_TYPE_ALL};

namespace Leviathan{

	class Event : public Object{
	public:
		DLLEXPORT Event::Event();
		DLLEXPORT Event::Event(EVENT_TYPE type, void* data);
		DLLEXPORT Event::Event(EVENT_TYPE type, void* data, bool allowdelete);
		DLLEXPORT Event::~Event();

		EVENT_TYPE Type;
		void* Data;
		bool DeleteIt : 1;

		DLLEXPORT EVENT_TYPE GetType();

	};

}
#endif