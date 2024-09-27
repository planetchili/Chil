#pragma once
#include <Core/src/win/ChilWin.h>
#include <Core/src/ioc/Container.h> 
#include <Core/src/log/SeverityLevelPolicy.h> 
#include <Core/src/log/Log.h> 
#include <Core/src/win/BootWin.h>
#include <Core/src/gfx/IResourceLoader.h>
#include <Core/src/gfx/ISpriteBatcher.h>
#include <Core/src/gfx/d12/BootD12.h>
#include <Core/src/log/Log.h> 
#include <Core/src/win/IWindow.h>
#include <Core/src/crn/RangeBits.h>
#include <Core/src/win/BootWin.h>
#include <Core/src/gfx/d12/BootD12.h>
#include <objbase.h>

namespace chil::simp
{
	class SimpleContext
	{
	public:
		static void Init(const spa::DimensionsI& windowDims, std::wstring windowName, log::Level logLevel)
		{
			if (auto& simp = SimpleContext::Get(); !simp.initialized_) {
				// boot ioc components
				Boot_(logLevel);
				// ioc container shortcut
				auto& C = ioc::Get();
				// make window
				simp.pKeyboard_ = std::make_shared<win::Keyboard>();
				simp.pWindow_ = C.Resolve<win::IWindow>(win::IWindow::IocParams{
					.pKeySink = simp.pKeyboard_,
					.name = std::move(windowName),
					.size = windowDims,
				});
				// make graphics pane
				simp.pPane_ = C.Resolve<gfx::IRenderPane>(gfx::IRenderPane::IocParams{
					.hWnd = simp.pWindow_->GetHandle(),
					.dims = windowDims,
				});
				// create sprite codex
				simp.pSpriteCodex_ = C.Resolve<gfx::ISpriteCodex>({ 32 });
				// create resource loader
				simp.pResourceLoader_ = C.Resolve<gfx::IResourceLoader>();
				// make sprite batcher
				simp.pSpriteBatcher_ = C.Resolve<gfx::ISpriteBatcher>(gfx::ISpriteBatcher::IocParams{
					.targetDimensions = windowDims,
					.pSpriteCodex = simp.pSpriteCodex_,
					.maxSpriteCount = 10,
				});
				// we are done
				simp.initialized_ = true;
			}
			else {
				chilog.warn(L"Multiple simp::Init calls detected");
			}
		}
		~SimpleContext()
		{
			if (initialized_) {
				pPane_->FlushQueues();
			}
		}
		static SimpleContext& Get()
		{
			static SimpleContext ctx;
			return ctx;
		}
		std::shared_ptr<gfx::ISpriteCodex::Atlas> LoadAtlas(const std::wstring& path)
		{
			auto pTex = pResourceLoader_->LoadTexture(path).get();
			return pSpriteCodex_->AddAtlas(std::move(pTex));
		}
		gfx::ISpriteBatcher& GetBatcher()
		{
			return *pSpriteBatcher_;
		}
		win::IWindow& GetWindow()
		{
			return *pWindow_;
		}
		win::IKeyboardSource& GetKeyboard()
		{
			return *pKeyboard_;
		}
		void BeginFrame()
		{
			pPane_->BeginFrame();
			pSpriteBatcher_->StartBatch(*pPane_);
		}
		void EndFrame()
		{
			pSpriteBatcher_->EndBatch(*pPane_);
			pPane_->EndFrame();
			// grow batcher's capacities if needed, also collect any ready garbage
			pSpriteBatcher_->CollectGarbage(*pPane_);
			if (pSpriteBatcher_->GetDrawCount() > pSpriteBatcher_->GetCapacity()) {
				const auto newCapacity = UINT(float(pSpriteBatcher_->GetDrawCount()) * 1.5f);
				chilog.debug(std::format(L"Growing batcher {} => {}", pSpriteBatcher_->GetCapacity(), newCapacity));
				pSpriteBatcher_->Reserve(UINT(float(pSpriteBatcher_->GetDrawCount()) * 1.5f));
			}
		}
	private:
		// functions
		static void Boot_(log::Level logLevel)
		{
			// boot logging system
			log::Boot();
			ioc::Get().Register<log::ISeverityLevelPolicy>([logLevel] {
				return std::make_shared<log::SeverityLevelPolicy>(logLevel);
			});
			// init COM
			if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
				throw std::runtime_error{ "COM farked" };
			}
			// boot window system
			win::Boot();
			// boot d3d12 graphics system
			gfx::d12::Boot();
		}
		// data
		bool initialized_ = false;
		std::shared_ptr<win::Keyboard> pKeyboard_;
		std::shared_ptr<win::IWindow> pWindow_;
		std::shared_ptr<gfx::IRenderPane> pPane_;
		std::shared_ptr<gfx::ISpriteBatcher> pSpriteBatcher_;
		std::shared_ptr<gfx::ISpriteCodex> pSpriteCodex_;
		std::shared_ptr<gfx::IResourceLoader> pResourceLoader_;
	};

	inline void Init(const spa::DimensionsI& windowDims, std::wstring windowName, log::Level logLevel = log::Level::Error)
	{
		SimpleContext::Init(windowDims, std::move(windowName), logLevel);
	}
	inline std::shared_ptr<gfx::ISpriteCodex::Atlas> LoadAtlas(const std::wstring& path)
	{
		return SimpleContext::Get().LoadAtlas(path);
	}
	inline gfx::ISpriteBatcher& Batch()
	{
		return SimpleContext::Get().GetBatcher();
	}
	inline win::IWindow& Win()
	{
		return SimpleContext::Get().GetWindow();
	}
	inline win::IKeyboardSource& Kbd()
	{
		return SimpleContext::Get().GetKeyboard();
	}
	inline void Begin()
	{
		return SimpleContext::Get().BeginFrame();
	}
	inline void End()
	{
		return SimpleContext::Get().EndFrame();
	}
}