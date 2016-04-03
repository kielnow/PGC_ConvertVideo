#pragma once

namespace app
{

	using namespace zen;

	class ConvertVideo : public App
	{
	public:
		typedef App super;

		ConvertVideo();

		virtual ~ConvertVideo();

		virtual void initialize() override;

		virtual void finalize() override;

		virtual void update() override;

		virtual void draw(ID2D1RenderTarget* pRT) override;

	protected:
	};

}
