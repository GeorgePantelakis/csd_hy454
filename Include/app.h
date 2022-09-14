#pragma once

#include <functional>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <math.h>
#include <string>
#include <unordered_set>
#include <cstdlib>
#include <ctime>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5\allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include "Defines.h"
#include "Typedefs.h"
#include "TileLayer.h"

using namespace std;

//--------------------STRUCTS---------------------------

namespace app {
	using BitmapAccessFunctor = std::function<void(unsigned char**)>;

	extern unsigned long currTime;

	//--------------------TYPEDEFS 2------------------------
	typedef RGB Palette[256];

	//--------------------CLASSES---------------------------
	class Game {
		public:
			using Action = std::function<void(void)>;
			using Pred = std::function<bool(void)>;
		private:
			Action render, anim, input, ai, physics, destruct, collisions, user;
			Pred done;

			Action pauseResume;
			bool isPaused = false;
			uint64_t pauseTime = 0;

			int lives = 6;
			int coins = 0;
			int points = 0;

			void Invoke(const Action& f);
		public:
			void SetOnPauseResume(const Action& f) { pauseResume = f; }
			template <typename Tfunc> void SetDone(const Tfunc& f) { done = f; }
			template <typename Tfunc> void SetRender(const Tfunc& f) { render = f; }
			template <typename Tfunc> void SetInput(const Tfunc& f) { input = f; }
			template <typename Tfunc> void SetProgressAnimations(const Tfunc& f) { anim = f; }
			template <typename Tfunc> void SetPhysics(const Tfunc& f) { physics = f; }
			template <typename Tfunc> void SetCollisionCheck(const Tfunc& f) { collisions = f; }
			template <typename Tfunc> void SetAI(const Tfunc& f) { ai = f; }
			// rest of setters are similary defined

			void setInitLifes(int l);
			void Render(void);
			void ProgressAnimations(void);
			void Input(void);
			void AI(void);
			void Physics(void);
			void CollisionChecking(void);
			void CommitDestructions(void);
			void UserCode(void);
			bool IsFinished(void) const;
			void MainLoop(void);
			void MainLoopIteration(void);

			void Pause(uint64_t t);
			void Resume(void);
			bool IsPaused(void) const;
			uint64_t GetPauseTime(void) const;

			int getLives(void);
			void addLife(void);
			void loseLife(void);
			bool isGameOver(void);

			int getCoins(void);
			void addCoin(void);
			void resetCoins(void);

			int getPoints(void);
			void addPoints(int);
			void addPointsNoShow(int);
	};


	class App {
		protected:
			Game game;

		public:
			virtual void		Initialise (void) = 0;
			virtual void		Load (void) = 0;
			virtual void		Run(void);
			virtual void		RunIteration(void);
			Game&				GetGame(void);
			const Game&			GetGame(void) const;
			virtual void		Clear (void) = 0;
			void Main(void);
	};
	
	//bool ALLEGRO_COLOR::operator<(const Color& c1, const Color& c2) { return true; }

	class MainApp : public App {
		public:
			void	Initialise(void);
			void	Load(void);
			void	Clear(void);
	};

	/*
	class TileColorsHolder final {
	private:
		std::set<Index> indices;
		std::set<Color> colors;
	public:
		void Insert(Bitmap bmp, Index index);
		bool In(Color c) const;
	};*/

	//---------Color------------
	extern void SetPalette(RGB* palette);
	extern Color Make8(RGBValue r, RGBValue g, RGBValue b);
	extern Color Make16(RGBValue r, RGBValue g, RGBValue b);
	extern Color Make24(RGBValue r, RGBValue g, RGBValue b);
	extern Color Make32(RGBValue r, RGBValue g, RGBValue b, Alpha alpha);
	void ReadPixelColor32(PixelMemory mem, RGBA *c, Alpha *a);
	Color GetPixel32(PixelMemory mem);

	bool ReadTextMap(class TileLayer* layer, string filename);

	int GetMapPixelWidth(void);
	int GetMapPixelHeight(void);

	bool characterStaysInCenter(Rect pos, int* dx);
}

//--------------------OVERLOADED OPS--------------------

bool operator<(const Color left, const Color right);

void SetGameTime();
unsigned long GetGameTime();