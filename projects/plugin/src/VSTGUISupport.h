
#ifndef VSTGUISupport_H
	#define VSTGUISupport_H
	#include "Common.h"
	#ifdef APE_VST
	class CRunFileDelegate 

			: public CBaseObject
		{
			SciLexer * parent;
			volatile int operationResult;
			std::vector<std::string> files;
		public:
			CRunFileDelegate(SciLexer * s)
				: parent(s), operationResult(0)
			{

			}

			VSTGUI::CMessageResult notify(CBaseObject * sender, IdStringPtr ID)
			{
				// this api seems really sketchy. comparing strings by pointers????
				if (ID == CNewFileSelector::kSelectEndMessage)
				{
					CNewFileSelector* sel = dynamic_cast<CNewFileSelector*>(sender);
					if (sel)
					{
						int numFiles = sel->getNumSelectedFiles();
						if (!numFiles)
							goto error;
						for (int i = 0; i < numFiles; i++)
							files.push_back(sel->getSelectedFile(i));
						// do anything with the selected files here
						operationResult = 1;
						return kMessageNotified;
					}
				error:
					operationResult = -1;
					return kMessageNotified;
				}
				// no parent though
				//return parent::notify(sender, message);
				//operationResult = -1;
				return CMessageResult::kMessageUnknown;
			}

			bool join()
			{
		#pragma cwarn("Potential deadlock here when using file dialogs: might want to implement SpinLock() here.")
				while (!operationResult)
					APE::Misc::Delay(10);
				if (operationResult > 0)
					return true;
				else
					return false;
			}
			std::vector<std::string> & getFiles() { return files; }
		};
		class CLineContainer
			: public CScrollView
		{
			CConsole * parent;
		public:
			//virtual bool onWheel (CDrawContext *pContext, const CPoint &where, const CMouseWheelAxis axis, float distance);
			//virtual bool onWheel (CDrawContext *pContext, const CPoint &where, float distance);
			//virtual void mouse(CDrawContext *pContext, CPoint &where, long buttons);
			CLineContainer(CConsole * p, const CRect &rect, const CRect &fakeSize, CFrame *pParent, CBitmap *pBackground = (CBitmap*)0);
			void setScroll(float val);
		};
	#endif
#endif