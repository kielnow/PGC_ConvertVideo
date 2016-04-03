#pragma once

namespace zen
{

	class Graphics2D
	{
	public:
		static Graphics2D* getInstance() { return gInstance; }

		static void initialize();

		static void finalize();

		ID2D1Factory* getFactory() { return mpFactory; }

	protected:
		static Graphics2D* gInstance;

		Graphics2D();

		~Graphics2D();

		ID2D1Factory* mpFactory;
	};

}

#define IGraphics2D		(zen::Graphics2D::getInstance())
