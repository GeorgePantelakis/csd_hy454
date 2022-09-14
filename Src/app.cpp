#include "../Include/app.h"
#include "../Include/Bitmap.h"
#include "../Include/Utilities.h"
#include "../Include/Animation.h"
#include "../Include/Animator.h"
#include "../Include/createNPCs.h"
#include "../Include/Sprite.h"

using namespace std;

struct game_params game_params;

//--------------------GLOBAL VARS-----------------------
class TileLayer* action_layer;
TileLayer* underground_layer;
class CircularBackground* circular_background;
class CircularBackground* menu_circular_background;
std::unordered_map<std::string, std::vector<std::string>> enemies_positions;

Rect displayArea = Rect{ 0, 0, DISP_AREA_X, DISP_AREA_Y };
int widthInTiles = 0, heightInTiles = 0;
unsigned int total_tiles;
bool closeWindowClicked = false;
bool keys[ALLEGRO_KEY_MAX] = { 0 };
bool gridOn = true;
bool disable_input = false;
Bitmap characters = nullptr;
Bitmap npcs = nullptr;

ALLEGRO_TIMER* timer;
ALLEGRO_TIMER* fallingTimer;
ALLEGRO_TIMER* aiTimer;
ALLEGRO_TIMER* blockTimer;
ALLEGRO_TIMER* finishTimer;
ALLEGRO_TIMER* showTimer;

ALLEGRO_EVENT_QUEUE* queue;
ALLEGRO_EVENT_QUEUE* fallingQueue;
ALLEGRO_EVENT_QUEUE* aiQueue;
ALLEGRO_EVENT_QUEUE* blockQueue;
ALLEGRO_EVENT_QUEUE* finishQueue;
ALLEGRO_EVENT_QUEUE* showQueue;

ALLEGRO_DISPLAY* display;
bool scrollEnabled = false;
int mouse_x = 0, mouse_y = 0, prev_mouse_x = 0, prev_mouse_y = 0;
ALLEGRO_MOUSE_STATE mouse_state;
ALLEGRO_EVENT event;

class BitmapLoader* bitmaploader;

class Sprite* mario; //replace mario with SpriteManager::GetSingleton().GetTypeList("mario").front()
class MovingAnimator* walk, * pipe_movement;
class FrameRangeAnimator* jump;
bool not_moved = true;
bool jumped = false;
class FrameRangeAnimation* jump_anim = nullptr;

bool isDead = false;
bool winFinished = false;
bool respawing = false;
bool once = true;
int secondsToClose = 2;
int checkpoint_x = 0;
int end_x = 0;

std::unordered_set <Sprite*> shells;

Bitmap liveIcon = nullptr;
Bitmap coinIcon = nullptr;

ALLEGRO_FONT* font;
ALLEGRO_FONT* paused_font;
ALLEGRO_FONT* tittle_font;
ALLEGRO_FONT* tittle_font_smaller;

list<struct pointShow*> pointsShowList;

ALLEGRO_SAMPLE* deathEffect;
static ALLEGRO_SAMPLE* jumpEffect;
static ALLEGRO_SAMPLE* bgSong;
ALLEGRO_SAMPLE_INSTANCE* backgroundSong;
/*--------------------CLASSES---------------------------*/

//-------------Class Game----------------
void app::Game::setInitLifes(int l) {
	lives = l;
}

void app::Game::Invoke(const Action& f) { if (f) f(); }
void app::Game::Render(void) { Invoke(render); }
void app::Game::ProgressAnimations(void) { Invoke(anim); }
void app::Game::Input(void) { Invoke(input); }
void app::Game::AI(void) { Invoke(ai); }
void app::Game::Physics(void) { Invoke(physics); }
void app::Game::CollisionChecking(void) { Invoke(collisions); }
void app::Game::CommitDestructions(void) { Invoke(destruct); }
void app::Game::UserCode(void) { Invoke(user); }
bool app::Game::IsFinished(void) const { return !done(); }
void app::Game::MainLoop(void) {
	while (!IsFinished())
		MainLoopIteration();
}

unsigned long app::currTime;

void SetGameTime() {
	app::currTime = GetSystemTime();
}

unsigned long GetGameTime() {
	return app::currTime;
}

void app::Game::MainLoopIteration(void) {
	SetGameTime();
	Input();
	if (!IsPaused()) {
		Render();
		ProgressAnimations();
		AI();
		Physics();
		CollisionChecking();
		//DestructionManager::Get().Commit();
		CommitDestructions();
		UserCode(); // hook for custom code at end
	}
}

//-------------Class APP----------------

void app::App::RunIteration(void) {
	game.MainLoopIteration();
}

app::Game& app::App::GetGame(void) {
	return game;
}

const app::Game& app::App::GetGame(void) const { 
	return game; 
}

void InstallPauseResumeHandler(Game& game) {
	game.SetOnPauseResume(
		[&game](void) {
			if (!game.IsPaused()) { // just resumed
				AnimatorManager::GetSingleton().TimeShift(
					GetGameTime() - game.GetPauseTime()
				);
				al_flush_event_queue(fallingQueue);
				al_flush_event_queue(aiQueue);
				al_flush_event_queue(showQueue);
			}
			else {
				if(!game.isGameOver() && !respawing && !winFinished)
					al_draw_text(paused_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, action_layer->GetViewWindow().h / 2, ALLEGRO_ALIGN_CENTER, "Paused");
				al_flip_display();
			}
		}
	);
}

void loadMap(TileLayer *layer, string path) {
	app::ReadTextMap(layer, path);
}

//--------------------------------------

void loadSolidTiles(ALLEGRO_CONFIG* config, TileLayer *layer) {
	string text = "";
	vector<string> tiles;

	text = al_get_config_value(config, "tiles", "solid");
	tiles = splitString(text, " ");
	for (auto tile : tiles) {
		layer->insertSolid(atoi(tile.c_str()));
	}
	
}

void Sprite_MoveAction(Sprite* sprite, const MovingAnimation& anim) {
	sprite->Move(anim.GetDx(), anim.GetDy());
	sprite->NextFrame();
}

void FrameRange_Action(Sprite* sprite, Animator* animator, FrameRangeAnimation& anim) {
	auto* frameRangeAnimator = (FrameRangeAnimator*)animator;
	sprite->GetBox();
	if (frameRangeAnimator->GetCurrFrame() != anim.GetStartFrame() || frameRangeAnimator->GetCurrRep())
		sprite->Move(anim.GetDx(), anim.GetDy());

	sprite->SetFrame(frameRangeAnimator->GetCurrFrame());

	anim.ChangeSpeed(frameRangeAnimator->GetCurrFrame()); //changes Dx and Dy
}

static void createClosestEnemies(void) {
	for (std::pair<std::string, std::vector<std::string> > enemie_position : enemies_positions) {
		std::vector<std::string> copied_locations(enemie_position.second);
		for (std::vector<std::string>::iterator it = enemie_position.second.begin(); it != enemie_position.second.end(); ++it) {
			std::vector<std::string> coordinates = splitString(*it, " ");
			int x = std::stoi(coordinates[0]) - action_layer->GetViewWindow().x, y = std::stoi(coordinates[1]);
			if (x < action_layer->GetViewWindow().w) {
				Sprite* new_sprite = nullptr;
				if (enemie_position.first == "goomba")
					new_sprite = create_enemy_goomba(x, y);
				else if (enemie_position.first == "green_koopa_troopa")
					new_sprite = create_enemy_green_koopa_troopa(x, y);
				else if (enemie_position.first == "red_koopa_troopa")
					new_sprite = create_enemy_red_koopa_troopa(x, y);
				else if (enemie_position.first == "piranha_plant") {
					new_sprite = create_enemy_piranha_plant(x, y);
					Sprite *pipe = new Sprite(x - 8, y, AnimationFilmHolder::GetInstance().GetFilm("Pipe.up"), "pipe");
					pipe->SetHasDirectMotion(true);
					pipe->GetGravityHandler().setGravityAddicted(false);
					pipe->SetZorder(1);
					pipe->SetBoundingArea(new BoundingBox(pipe->GetBox().x, pipe->GetBox().y, pipe->GetBox().x + pipe->GetBox().w, pipe->GetBox().y + pipe->GetBox().h));
					pipe->SetFormStateId(PIPE);
					SpriteManager::GetSingleton().Add(pipe);
				}
				copied_locations.erase(std::find(copied_locations.begin(), copied_locations.end(), *it));
				if (new_sprite) {
					for (auto shell : shells) {
						CollisionChecker::GetSingleton().Register(shell, new_sprite, [](Sprite* s1, Sprite* s2) {
							if (s1->GetFormStateId() == SMASHED && s2->GetFormStateId() == SMASHED) //2 sells colide? do nothing
								return;
							if (s1->GetStateId() == IDLE_STATE) //if someone collides when shell is not moving
								return;
							if (s2->GetTypeId() == "piranha_plant" && ((MovingPathAnimation*)s2->GetAnimator()->GetAnim())->GetPath().size() == 1 + ((MovingPathAnimator*)s2->GetAnimator())->GetFrame())
								return; //if piranha is under the pipe do nothing
							s2->SetFormStateId(DELETE);
							//CollisionChecker::GetSingleton().Cancel(s1, s2);
							for (auto shell : shells) //deleting sprite so remove all collisions it has with the shells
								CollisionChecker::GetSingleton().Cancel(shell, s2);
							CollisionChecker::GetSingleton().Cancel(mario, s2);

							Animator* tmp = s2->GetAnimator();
							tmp->deleteCurrAnimation();
							tmp->Stop();
							AnimatorManager::GetSingleton().Cancel(tmp);
							tmp->Destroy();
						});
					}
				}
			}
		}
		enemies_positions[enemie_position.first] = copied_locations;
	}
}

void recreateSprites(ALLEGRO_CONFIG* config, Game& game, bool checkpoint) {
	walk = new MovingAnimator();
	jump = new FrameRangeAnimator();
	pipe_movement = new MovingAnimator();

	AnimatorManager::GetSingleton().Register(walk);
	AnimatorManager::GetSingleton().Register(jump);
	AnimatorManager::GetSingleton().Register(pipe_movement);
	walk->SetOnAction([](Animator* animator, const Animation& anim) {
		Sprite_MoveAction(mario, (const MovingAnimation&)anim);
		});

	jump->SetOnAction([](Animator* animator, const Animation& anim) {
		FrameRange_Action(mario, animator, (FrameRangeAnimation&)anim);
		});

	jump->SetOnStart([](Animator* animator) {
		mario->GetGravityHandler().SetFalling(false);
		mario->GetGravityHandler().setGravityAddicted(false);
		});
	jump->SetOnFinish([](Animator* animator) {
		delete jump_anim;
		jump_anim = nullptr;
		jump->Stop();
		mario->GetGravityHandler().setGravityAddicted(true);
		mario->GetGravityHandler().Check(mario->GetBox());
		mario->SetFrame(0);
		});

	pipe_movement->SetOnAction([](Animator* animator, const Animation& anim) {
		Sprite_MoveAction(mario, (const MovingAnimation&)anim);
		});
	pipe_movement->SetOnStart([](Animator* animator) {
		mario->SetHasDirectMotion(true);
		mario->GetGravityHandler().setGravityAddicted(false);
		mario->GetGravityHandler().SetFalling(false);
		mario->SetFrame(0);
		if (mario->lastMovedRight)
			mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_right"));
		else
			mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_left"));
		AnimatorManager::GetSingleton().MarkAsSuspended(walk);

		if (jump->IsAlive()) {
			jump->Stop();
		}
		disable_input = true;
		});

	walk->Start(new MovingAnimation("walk", 0, 0, 0, 80), GetGameTime());

	vector<string> coordinates = splitString(al_get_config_value(config, "potitions", "start"), " ");
	mario = new Sprite(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()), AnimationFilmHolder::GetInstance().GetFilm("Mario_small.stand_right"), "mario");
	SpriteManager::GetSingleton().Add(mario);

	circular_background->Scroll(-action_layer->GetViewWindow().x);
	underground_layer->SetViewWindow(Rect{ 0,0,action_layer->GetViewWindow().w,action_layer->GetViewWindow().h });
	action_layer->SetViewWindow(Rect{ 0,0,action_layer->GetViewWindow().w,action_layer->GetViewWindow().h });

	mario->SetMover([](const Rect& pos, int* dx, int* dy) {
		int old_dx = *dx;
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};
		if (gridOn)
			action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
		mario->SetPos(pos.x + *dx, pos.y + *dy);

		if (old_dx > * dx && old_dx > 0 || old_dx < *dx && old_dx < 0) {
			mario->SetStateId(IDLE_STATE);
		}
		});

	mario->SetBoundingArea(new BoundingBox(mario->GetBox().x, mario->GetBox().y, mario->GetBox().x + mario->GetBox().w, mario->GetBox().y + mario->GetBox().h));
	mario->SetFormStateId(SMALL_MARIO);
	mario->Set_Str_StateId("Mario_small");

	PrepareSpriteGravityHandler(action_layer->GetGrid(), mario);

	std::vector<std::string> enemies_names = { "goomba", "green_koopa_troopa", "red_koopa_troopa", "piranha_plant" };
	vector<string> locations;
	for (std::string enemie_name : enemies_names) {
		enemies_positions[enemie_name] = std::vector<std::string>();
		locations = splitString(al_get_config_value(config, "emenies_positions", enemie_name.c_str()), ",");
		for (auto location : locations)
			enemies_positions[enemie_name].push_back(location);
	}

	//create all pipe sprites and add collisions
	for (auto pipes : splitString(al_get_config_value(config, "pipes", "pipe_locations"), ",")) {

		Sprite* pipe = LoadPipeCollision(mario, pipes);
		pipe->SetFormStateId(PIPE);
		SpriteManager::GetSingleton().Add(pipe);

	}

	//create super mushroom
	locations = splitString(al_get_config_value(config, "powerups_positions", "super"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_super_mushroom(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()));
	}

	//create 1up mushroom
	locations = splitString(al_get_config_value(config, "powerups_positions", "1up"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_1UP_mushroom(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()), &game);
	}

	//create star
	locations = splitString(al_get_config_value(config, "powerups_positions", "star"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_starman(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()));
	}

	for (unsigned int i = 0; i < action_layer->GetMapWidth(); i++) {
		for (unsigned int j = 0; j < action_layer->GetMapHeight(); j++) {
			// replace each brick index with sprite
			if (action_layer->GetTile(j, i) == 0) {
				create_block_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), &game);
			}
			else

			// replace each block index with sprite
			if (action_layer->GetTile(j, i) == 4) {
				create_brick_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j));
			}
			else

			// replace each coin index with sprite
			if (action_layer->GetTile(j, i) == 26) {
				create_coin_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), &game);
			}
			else
				//create flag pole
				if (action_layer->GetTile(j, i) == 496 || action_layer->GetTile(j, i) == 470) { //this is the final flag
					Sprite* pole = nullptr;
					if (action_layer->GetTile(j, i) == 496)
						pole = new Sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), AnimationFilmHolder::GetInstance().GetFilm("blocks.pole"), "pole");
					else
						pole = new Sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), AnimationFilmHolder::GetInstance().GetFilm("blocks.green_ball"), "pole");
					SpriteManager::GetSingleton().Add(pole);
					pole->SetHasDirectMotion(true);
					pole->GetGravityHandler().setGravityAddicted(false);
					pole->SetBoundingArea(new BoundingBox(pole->GetBox().x, pole->GetBox().y, pole->GetBox().x + pole->GetBox().w, pole->GetBox().y + pole->GetBox().h));
					CollisionChecker::GetSingleton().Register(mario, pole, [&game](Sprite* s1, Sprite* s2) {
						disable_input = true;
						mario->SetVisibility(true);
						CollisionChecker::GetSingleton().CancelAll(mario);
						if (mario->won) return;
						mario->won = true;
						AnimatorManager::GetSingleton().CancelAndRemoveAll();

						mario->SetFrame(0);
						mario->SetStateId(WALKING_STATE);
						mario->GetGravityHandler().setGravityAddicted(false);
						mario->GetGravityHandler().SetFalling(false);
						mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".winning_sequence"));

						MovingAnimator* finish_sequence = new MovingAnimator();
						mario->Move(7, 0);
						finish_sequence->SetOnAction([&game](Animator* animator, const Animation& anim) {
							Sprite_MoveAction(mario, (const MovingAnimation&)anim);
							game.addPointsNoShow(10);
							if (mario->GetGravityHandler().isOnSolidGround(mario->GetBox())) {
								animator->Stop();
							}
						});

						finish_sequence->SetOnFinish([](Animator* animator) { //when we get here, mario is on the ground. stand walking right
							cout << "CONGRATZ!\n";
							//animator->deleteCurrAnimation();
							mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".walk_right"));
							mario->SetFrame(0);
							mario->GetGravityHandler().setGravityAddicted(true);

							//start walking right
							animator->SetOnAction([](Animator* animator, const Animation& anim) {
								Sprite_MoveAction(mario, (const MovingAnimation&)anim);
								});

							((MovingAnimator*)animator)->Start(new MovingAnimation("finish_sequence", 80, 2, 0, 20), GetGameTime());
							animator->SetOnFinish([](Animator* animator) {
								winFinished = true;
								mario->SetVisibility(false);
								AnimatorManager::GetSingleton().Cancel(animator);
								});
							});

						AnimatorManager::GetSingleton().Register(finish_sequence);
						finish_sequence->Start(new MovingAnimation("finish_sequence", 0, 0, 1, 20), GetGameTime());

				});
			}
		}
	}

	for (auto brick : SpriteManager::GetSingleton().GetTypeList("brick")) {
		action_layer->SolidTileGridBlock(DIV_TILE_WIDTH(brick->GetBox().x + action_layer->GetViewWindow().x), DIV_TILE_HEIGHT(brick->GetBox().y + action_layer->GetViewWindow().y));
	}

	if (checkpoint) {
		MoveScene(checkpoint_x - 100, 0, 100, atoi(coordinates[1].c_str()));
	}
}

void respawn(Game& game) {
	ALLEGRO_CONFIG* config = al_load_config_file(".\\Engine\\config.ini");
	bool checkpoint = false;



	if (mario->GetBox().x + action_layer->GetViewWindow().x >= checkpoint_x && mario->GetBox().x + action_layer->GetViewWindow().x < end_x) {
		checkpoint = true;
	}

	if (jump_anim != nullptr) {
		jump->Stop();
		delete jump_anim;
	}

	CollisionChecker::GetSingleton().clear();
	AnimatorManager::GetSingleton().CancelAndRemoveAll();
	SpriteManager::GetSingleton().RemoveAll();
	shells.clear();

	recreateSprites(config, game, checkpoint);

	mario->Move(1, 0);
}

void InitialiseParams(ALLEGRO_CONFIG*  config) {

	game_params.initial_player_lifes = atoi(al_get_config_value(config, "Game_Params", "initial_player_lifes"));
	game_params.invinsible_mario_duration = atoi(al_get_config_value(config, "Game_Params", "invinsible_mario_duration"));
	game_params.coins_to_get_life = atoi(al_get_config_value(config, "Game_Params", "coins_to_get_life"));
	game_params.koopa_troopa_stun_duration = atoi(al_get_config_value(config, "Game_Params", "koopa_troopa_stun_duration"));
	game_params.enemy_speed = atoi(al_get_config_value(config, "Game_Params", "enemy_speed"));
	game_params.enemy_shell_speed = atoi(al_get_config_value(config, "Game_Params", "enemy_shell_speed"));
	game_params.goomba_points = atoi(al_get_config_value(config, "Game_Params", "goomba_points"));
	game_params.green_koopa_troopa_points = atoi(al_get_config_value(config, "Game_Params", "green_koopa_troopa_points"));
	game_params.red_koopa_troopa_points = atoi(al_get_config_value(config, "Game_Params", "red_koopa_troopa_points"));
	game_params.piranha_plant_points = atoi(al_get_config_value(config, "Game_Params", "piranha_plant_points"));
	game_params.powerups_points = atoi(al_get_config_value(config, "Game_Params", "powerups_points"));
	game_params.mario_max_speed = atoi(al_get_config_value(config, "Game_Params", "mario_max_speed"));
	game_params.coin_points = atoi(al_get_config_value(config, "Game_Params", "coin_points"));
}

void InitialiseGame(Game& game) {

	InstallPauseResumeHandler(game);

	game.SetDone(
		[&game](void) {
			if (isDead) {
				game.loseLife();
				al_stop_sample_instance(backgroundSong);
				al_play_sample(deathEffect, .5f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
				al_start_timer(finishTimer);
				game.Pause(GetGameTime());
				disable_input = true;
				isDead = false;
				if (game.isGameOver()) {
					secondsToClose = 30;
					mario->SetVisibility(false);
				}
				else {
					secondsToClose = 3;
					respawn(game);
					respawing = true;
				}
				isDead = false;
			}
			if (respawing) {
				game.Render();

				al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 10, ALLEGRO_ALIGN_CENTER, "You DIED!");
				al_draw_text(paused_font, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, action_layer->GetViewWindow().h / 2, ALLEGRO_ALIGN_CENTER, "You DIED!");
				al_draw_text(tittle_font_smaller, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 60, ALLEGRO_ALIGN_CENTER, ("You will be respawned in " + to_string(secondsToClose) + " seconds").c_str());

				al_flip_display();

				if (!al_is_event_queue_empty(finishQueue)) {
					al_wait_for_event(finishQueue, &event);
					secondsToClose--;
					if (secondsToClose == 0) {
						game.Resume();
						al_stop_timer(finishTimer);
						al_play_sample_instance(backgroundSong);
						disable_input = false;
						respawing = false;
					}
				}
			}
			if (game.isGameOver()) {
				game.Render();

				al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) - 40, ALLEGRO_ALIGN_CENTER, "Game Over! :'(");
				al_draw_text(paused_font, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) - 50, ALLEGRO_ALIGN_CENTER, "Game Over! :'(");
				al_draw_text(tittle_font_smaller, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 10, ALLEGRO_ALIGN_CENTER, ("Your score: " + to_string(game.getPoints())).c_str());
				al_draw_text(tittle_font_smaller, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 50, ALLEGRO_ALIGN_CENTER, ("The window will close in " + to_string(secondsToClose) + " seconds").c_str());

				al_flip_display();

				if (!al_is_event_queue_empty(finishQueue)) {
					al_wait_for_event(finishQueue, &event);
					secondsToClose--;
					if (secondsToClose == 0) {
						return false;
					}
				}
			}
			if (winFinished) {

				if (once) {
					secondsToClose = 30;
					al_start_timer(finishTimer);
					game.Pause(GetGameTime());
					mario->SetVisibility(false);
					once = false;
				}

				game.Render();

				al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) - 40, ALLEGRO_ALIGN_CENTER, "You won the game!");
				al_draw_text(paused_font, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) - 50, ALLEGRO_ALIGN_CENTER, "You won the game!");
				al_draw_text(tittle_font_smaller, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 10, ALLEGRO_ALIGN_CENTER, ("Your score: " + to_string(game.getPoints())).c_str());
				al_draw_text(tittle_font_smaller, al_map_rgb(255, 255, 255), action_layer->GetViewWindow().w / 2, (action_layer->GetViewWindow().h / 2) + 50, ALLEGRO_ALIGN_CENTER, ("The window will close in " + to_string(secondsToClose) + " seconds").c_str());

				al_flip_display();

				if (!al_is_event_queue_empty(finishQueue)) {
					al_wait_for_event(finishQueue, &event);
					secondsToClose--;
					if (secondsToClose == 0) {
						return false;
					}
				}
			}
			return !closeWindowClicked;
		}
	);

	game.SetRender(
		[&game](void) {
			circular_background->Display(al_get_backbuffer(display), displayArea.x, displayArea.y);
			underground_layer->TileTerrainDisplay(al_get_backbuffer(display), displayArea);
			action_layer->TileTerrainDisplay(al_get_backbuffer(display), displayArea);

			for (auto sprite : SpriteManager::GetSingleton().GetDisplayList()) {
				sprite->Display(BitmapGetScreen());
			}
			
			al_draw_text(font, al_map_rgb(255, 255, 255), 30, 18, ALLEGRO_ALIGN_CENTER, "Lives: ");
			if (game.getLives() < 6) {
				for (int i = 0; i < game.getLives(); i++)
					al_draw_scaled_bitmap(liveIcon, 0, 0, 16, 16, 50 + (i * 15), 20, 14, 14, 0);
			}
			else {
				al_draw_scaled_bitmap(liveIcon, 0, 0, 16, 16, 50, 20, 14, 14, 0);
				al_draw_text(font, al_map_rgb(255, 255, 255), 70, 19, ALLEGRO_ALIGN_LEFT, ("x" + to_string(game.getLives())).c_str());
			}

			al_draw_text(font, al_map_rgb(255, 255, 255), 290, 18, ALLEGRO_ALIGN_CENTER, "Coins: ");
			al_draw_scaled_bitmap(coinIcon, 0, 0, 16, 16, 310, 18, 16, 16, 0);
			al_draw_text(font, al_map_rgb(255, 255, 255), 325, 19, ALLEGRO_ALIGN_LEFT, ("x" + to_string(game.getCoins())).c_str());
			
			al_draw_text(font, al_map_rgb(255, 255, 255), 525, 18, ALLEGRO_ALIGN_CENTER, "Score: ");
			al_draw_text(font, al_map_rgb(255, 255, 255), 550, 19, ALLEGRO_ALIGN_LEFT, standarizeSize(to_string(game.getPoints()), 8).c_str());

			list<struct pointShow*> toBeDeleted;
			for (auto entry : pointsShowList) {
				al_draw_text(font, al_map_rgb(255, 255, 255), entry->x, entry->y, ALLEGRO_ALIGN_LEFT, to_string(entry->points).c_str());
				if (entry->y < entry->final_y) {
					toBeDeleted.push_back(entry);
				}
			}
			if (!al_is_event_queue_empty(showQueue)) {
				al_wait_for_event(showQueue, &event);
				for (auto entry : pointsShowList) {
					entry->y--;
					if (entry->y < entry->final_y) {
						toBeDeleted.push_back(entry);
					}
				}
			}
			for (auto entry : toBeDeleted) {
				pointsShowList.remove(entry);
			}

			if(!game.isGameOver() && !winFinished && !respawing)
				al_flip_display();
		}
	);

	game.SetInput(
		[&game] {
			if (!al_is_event_queue_empty(queue)) {
				al_wait_for_event(queue, &event);

				if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_TILDE) {
					gridOn = !gridOn;
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_P) {
					if (!game.isGameOver() && !winFinished) {
						if (game.IsPaused())
						game.Resume();
					else
						game.Pause(GetGameTime());
					}
					
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_K) {
					game.addLife();
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_L) {
					game.loseLife();
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_M) {
					game.addCoin();
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_N) {
					game.addPoints(100);
				}
				else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
					keys[event.keyboard.keycode] = true;
				}
				else if (event.type == ALLEGRO_EVENT_KEY_UP) {
					keys[event.keyboard.keycode] = false;
				}
				else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
					closeWindowClicked = true;
				}
				if (event.type == ALLEGRO_EVENT_TIMER) {
					if (game.IsPaused() || disable_input)
						return;
					not_moved = true;
					if (keys[ALLEGRO_KEY_W] || keys[ALLEGRO_KEY_UP]) {
						if (jump_anim == nullptr && !mario->GetGravityHandler().isFalling()) {
							al_play_sample(jumpEffect, .10f, 0.0f, 1.0f, ALLEGRO_PLAYMODE_ONCE, NULL);
							jump_anim = new FrameRangeAnimation("jump", 0, 17, 1, 0, -16, 15); //start, end, reps, dx, dy, delay

							jump_anim->SetChangeSpeed([](int& dx, int& dy, int frameNo) {
								//HERE CHAGNE DX AND DY DEPENDING ON FRAMENO
								int sumOfNumbers = 0;
								char maxTiles;

								if (mario->GetStateId() == RUNNING_STATE) {
									maxTiles = 5;
								}
								else {
									maxTiles = 4;
								}

								for (int i = 1; i <= jump_anim->GetEndFrame(); i++) sumOfNumbers += i;

								if (keys[ALLEGRO_KEY_W] || keys[ALLEGRO_KEY_UP])
									dy = -customRound((float)((jump_anim->GetEndFrame() - frameNo) * maxTiles * TILE_HEIGHT) / sumOfNumbers);
								else
									jump->Stop();
								});
							jump->Start(jump_anim, GetGameTime());

							if (mario->lastMovedRight)
								mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_right"));
							else
								mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_left"));
						}
					}

					if (keys[ALLEGRO_KEY_A] || keys[ALLEGRO_KEY_LEFT]) {
						if (mario->lastMovedRight)
							mario->resetSpeed();
						else
							mario->incSpeed(GetGameTime());

						if (mario->GetBox().x > 0) {
							mario->Move(-mario->GetSpeed(), 0);
						}

						if (jump_anim == nullptr) {
							mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".walk_left"));
						}
						mario->lastMovedRight = false;
						not_moved = false;
					}
					if (keys[ALLEGRO_KEY_D] || keys[ALLEGRO_KEY_RIGHT]) {
						if (mario->GetBox().x + mario->GetBox().w < action_layer->GetViewWindow().w) {
							if (mario->lastMovedRight)
								mario->incSpeed(GetGameTime());
							else
								mario->resetSpeed();

							int move_x = mario->GetSpeed();
							int move_y = 0;
							mario->Move(move_x, 0);
							if (app::characterStaysInCenter(mario->GetBox(), &move_x)) {
								underground_layer->ScrollWithBoundsCheck(&move_x, &move_y);
								action_layer->ScrollWithBoundsCheck(&move_x, &move_y);
								circular_background->Scroll(move_x);

								auto sprites = SpriteManager::GetSingleton().GetDisplayList();
								sprites.remove(mario);

								for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) { 
									sprite->SetHasDirectMotion(true);
									sprite->Move(-move_x, 0);
									sprite->SetHasDirectMotion(false);
									sprites.remove(sprite);
								}

								if (move_x != 0) {	// move the sprites the opposite directions (f.e. pipes)
									for (auto sprite : sprites) {
										sprite->Move(-move_x, 0);
									}
								}

								mario->Move(-move_x, -move_y);
								//mario->SetPos(mario->GetBox().x - move_x, mario->GetBox().y - move_y);
							}
							if (jump_anim == nullptr) {
								mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".walk_right"));
							}
							mario->lastMovedRight = true;
							not_moved = false;
						}
					}

					if (not_moved && jump_anim == nullptr) { //im not moving
						mario->SetFrame(0);
						mario->SetStateId(WALKING_STATE);
						mario->resetSpeed();
						if (mario->lastMovedRight)
							mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_right"));
						else
							mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_left"));
					}
				}
			}
		}
	);

	game.SetPhysics(
		[](void) {
			int falling_dy;

			if (!al_is_event_queue_empty(fallingQueue)) {
				al_wait_for_event(fallingQueue, &event);
				for (auto sprite : SpriteManager::GetSingleton().GetDisplayList()) {
					if (sprite->GetGravityHandler().isFalling()) {
						sprite->GetGravityHandler().increaseFallingTimer();
						falling_dy = (int)(sprite->GetGravityHandler().getFallingTimer() / 4) * GRAVITY_G;
						sprite->GetGravityHandler().checkFallingDistance(falling_dy, sprite->GetBox());
						sprite->Move(0, falling_dy); //gravity move down
					}
				}
			}
		}
	);

	game.SetAI(
		[&game](void) {
			vector<Sprite*> toBeDestroyed;
			vector<Sprite*> toBeDestroyedByBlock;
			vector<Sprite*> toBeDestroyedWithoutPoints;

			if (mario->GetBox().y > action_layer->GetViewWindow().h + 10) {
				isDead = true;
			}

			if (!al_is_event_queue_empty(aiQueue)) {
				al_wait_for_event(aiQueue, &event);
				std::vector<std::string> npcs_types = { "goomba", "green_koopa_troopa", "red_koopa_troopa" };
				for(std::string enemie_type : npcs_types)
					for (auto enemie : SpriteManager::GetSingleton().GetTypeList(enemie_type)) {
						if (enemie_type == "goomba" && enemie->GetFormStateId() == SMASHED) {
							//if smashed do nothing (dont move it)
						}
						else if (enemie->GetFormStateId() == DELETE) {
							enemie->SetVisibility(false);
							toBeDestroyed.push_back(enemie);
						}
						else if (enemie->GetBox().y > (action_layer->GetViewWindow().y + action_layer->GetViewWindow().h)) {
							enemie->SetVisibility(false);
							toBeDestroyedWithoutPoints.push_back(enemie);
						}
						else if (enemie->GetFormStateId() == DELETE_BY_BLOCK) {
							enemie->SetVisibility(false);
							toBeDestroyedByBlock.push_back(enemie);
						}
						else {
							int speed = 0;
							if (enemie_type == "goomba")
								speed = game_params.enemy_speed;
							else if (enemie->GetStateId() == WALKING_STATE) {
								speed = game_params.enemy_speed;
								if (enemie->GetFormStateId() == SMASHED)
									speed = game_params.enemy_shell_speed;
							}
							enemie->Move(enemie->lastMovedRight ? speed : -speed, 0);
						}
					}

				npcs_types = { "piranha_plant", "coin"};
				for (std::string npc_type : npcs_types)
					for (auto npc : SpriteManager::GetSingleton().GetTypeList(npc_type)) {
						if (npc->GetFormStateId() == DELETE) {
							npc->SetVisibility(false);
							toBeDestroyed.push_back(npc);
						}
					}

				for (auto powerup : SpriteManager::GetSingleton().GetTypeList("powerup")) {
					if (powerup->GetFormStateId() == DELETE) {
						powerup->SetVisibility(false);
						toBeDestroyed.push_back(powerup);
					}
					else if (powerup->GetBox().y > (action_layer->GetViewWindow().y + action_layer->GetViewWindow().h)) {
						powerup->SetVisibility(false);
						toBeDestroyedWithoutPoints.push_back(powerup);
					}
					else
						powerup->Move(powerup->lastMovedRight ? POWERUPS_MOVE_SPEED : -POWERUPS_MOVE_SPEED, 0);
				}
				for (auto sprite : toBeDestroyed) {
					if (sprite->GetTypeId() == "goomba")
						game.addPoints(game_params.goomba_points);
					else if (sprite->GetTypeId() == "green_koopa_troopa")
						game.addPoints(game_params.green_koopa_troopa_points);
					else if (sprite->GetTypeId() == "red_koopa_troopa")
						game.addPoints(game_params.red_koopa_troopa_points);
					else if (sprite->GetTypeId() == "piranha_plant")
						game.addPoints(game_params.piranha_plant_points);
					else if (sprite->GetTypeId() == "powerup")
						game.addPoints(game_params.powerups_points);
					SpriteManager::GetSingleton().Remove(sprite);
					CollisionChecker::GetSingleton().CancelAll(sprite);
				}
				for (auto sprite : toBeDestroyedByBlock) {
					if (sprite->GetTypeId() == "goomba")
						game.addPoints(game_params.goomba_points);
					else if (sprite->GetTypeId() == "green_koopa_troopa")
						game.addPoints(game_params.green_koopa_troopa_points);
					else if (sprite->GetTypeId() == "red_koopa_troopa")
						game.addPoints(game_params.red_koopa_troopa_points);
					else if (sprite->GetTypeId() == "piranha_plant")
						game.addPoints(game_params.piranha_plant_points);
					sprite->GetAnimator()->Stop();
					sprite->GetAnimator()->deleteCurrAnimation();
					sprite->GetAnimator()->Destroy();
					AnimatorManager::GetSingleton().Cancel(sprite->GetAnimator());
					SpriteManager::GetSingleton().Remove(sprite);
					CollisionChecker::GetSingleton().CancelAll(sprite);
				}
				for (auto sprite : toBeDestroyedWithoutPoints) {
					if (sprite->GetFormStateId() == SMASHED)
						shells.erase(sprite);
					if (sprite->GetAnimator()) {
						sprite->GetAnimator()->Stop();
						if (sprite->GetAnimator()->GetAnim())
							sprite->GetAnimator()->deleteCurrAnimation();
						sprite->GetAnimator()->Destroy();
						AnimatorManager::GetSingleton().Cancel(sprite->GetAnimator());
					}
					SpriteManager::GetSingleton().Remove(sprite);
					CollisionChecker::GetSingleton().CancelAll(sprite);
				}
				toBeDestroyed.clear();
				toBeDestroyedByBlock.clear();
				toBeDestroyedWithoutPoints.clear();
			}

			if (!al_is_event_queue_empty(blockQueue)) {
				al_wait_for_event(blockQueue, &event);
				for (auto block : SpriteManager::GetSingleton().GetTypeList("block")) {
					if (block->GetFormStateId() == MOVED_BLOCK) {
						block->Move(0, 4);
						block->SetFormStateId(EMPTY_BLOCK);
						block->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("blocks.empty_block"));
						al_stop_timer(blockTimer);
					}
				}
				for (auto brick : SpriteManager::GetSingleton().GetTypeList("brick")) {
					if (brick->GetFormStateId() == MOVED_BLOCK) {
						brick->Move(0, 4);
						brick->SetFormStateId(BRICK);
						al_stop_timer(blockTimer);
					}
					else if (brick->GetFormStateId() == SMASHED) {
						toBeDestroyed.push_back(brick);
						action_layer->UnsolidTileGridBlock(DIV_TILE_WIDTH(brick->GetBox().x + action_layer->GetViewWindow().x), DIV_TILE_HEIGHT(brick->GetBox().y + action_layer->GetViewWindow().y) + 1);
					}
				}
				for (auto sprite : toBeDestroyed) {
					SpriteManager::GetSingleton().Remove(sprite);
					CollisionChecker::GetSingleton().CancelAll(sprite);
				}
				toBeDestroyed.clear();
			}
			createClosestEnemies();
		}
	);

	game.SetCollisionCheck(
		[](void) {
			CollisionChecker::GetSingletonConst().Check();
		}
	);

	game.SetProgressAnimations(
		[](void) {
			AnimatorManager::GetSingleton().Progress(GetGameTime());
		}
	);
}

void app::MainApp::Initialise(void) {
	srand((unsigned int) time(NULL));
	SetGameTime();
	if (!al_init()) {
		std::cout << "ERROR: Could not init allegro\n";
		assert(false);
	}
	al_install_audio();

	al_init_acodec_addon();
	al_init_image_addon();
	al_init_primitives_addon();
	al_init_font_addon();
	al_init_ttf_addon();

	al_set_new_display_flags(ALLEGRO_WINDOWED);
	display = al_create_display(displayArea.w, displayArea.h);
	al_set_window_title(display, "Super Mario... CSD");
	queue = al_create_event_queue();
	al_install_keyboard();
	al_install_mouse();
	al_register_event_source(queue, al_get_mouse_event_source());
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_register_event_source(queue, al_get_display_event_source(display));

	timer = al_create_timer(1.0 / 60);
	al_register_event_source(queue, al_get_timer_event_source(timer));
	al_start_timer(timer);

	fallingQueue = al_create_event_queue();
	fallingTimer = al_create_timer(1.0 / 60);
	al_register_event_source(fallingQueue, al_get_timer_event_source(fallingTimer));
	al_start_timer(fallingTimer);

	aiQueue= al_create_event_queue();
	aiTimer = al_create_timer(1.0 / 60);
	al_register_event_source(aiQueue, al_get_timer_event_source(aiTimer));
	al_start_timer(aiTimer);

	blockQueue = al_create_event_queue();
	blockTimer = al_create_timer(1.0/6);
	al_register_event_source(blockQueue, al_get_timer_event_source(blockTimer));

	finishQueue = al_create_event_queue();
	finishTimer = al_create_timer(1.0);
	al_register_event_source(finishQueue, al_get_timer_event_source(finishTimer));

	showQueue = al_create_event_queue();
	showTimer = al_create_timer(1.0 / 30);
	al_register_event_source(showQueue, al_get_timer_event_source(showTimer));
	al_start_timer(showTimer);

	font = al_load_font(".\\Engine\\Bin\\game_font.ttf", 20, NULL);
	paused_font = al_load_font(".\\Engine\\Bin\\game_font.ttf", 40, NULL);
	tittle_font = al_load_font(".\\Engine\\Bin\\game_font.ttf", 30, NULL);
	tittle_font_smaller = al_load_font(".\\Engine\\Bin\\game_font.ttf", 25, NULL);

	jumpEffect = al_load_sample(".\\Engine\\Bin\\marioJumpSoundEffect.wav");
	assert(jumpEffect);
	deathEffect = al_load_sample(".\\Engine\\Bin\\marioDeathSoundEffect.wav");
	assert(deathEffect);
	bgSong = al_load_sample(".\\Engine\\Bin\\SuperMarioBrosBackgroundMusic.ogg");
	assert(bgSong);
	backgroundSong = al_create_sample_instance(bgSong);
	assert(backgroundSong);
	al_reserve_samples(3);
	al_set_sample_instance_playmode(backgroundSong, ALLEGRO_PLAYMODE_LOOP);
	al_set_sample_instance_gain(backgroundSong, .10f);
	al_attach_sample_instance_to_mixer(backgroundSong, al_get_default_mixer());
	al_play_sample_instance(backgroundSong);

	InitialiseGame(game);

	//TODO delete this later we dont need it. Animation has one bitmaploader
	bitmaploader = new BitmapLoader();

	walk = new MovingAnimator();
	jump = new FrameRangeAnimator();
	pipe_movement = new MovingAnimator();

	AnimatorManager::GetSingleton().Register(walk);
	AnimatorManager::GetSingleton().Register(jump);
	AnimatorManager::GetSingleton().Register(pipe_movement);
	walk->SetOnAction([](Animator* animator, const Animation& anim) {
		Sprite_MoveAction(mario, (const MovingAnimation&)anim);
	});

	jump->SetOnAction([](Animator* animator, const Animation& anim) {
		FrameRange_Action(mario, animator, (FrameRangeAnimation&)anim);
	});

	jump->SetOnStart([](Animator* animator) {
		mario->GetGravityHandler().SetFalling(false);
		mario->GetGravityHandler().setGravityAddicted(false);
	});
	jump->SetOnFinish([](Animator* animator) {
		delete jump_anim;
		jump_anim = nullptr;
		jump->Stop();
		mario->GetGravityHandler().setGravityAddicted(true);
		mario->GetGravityHandler().Check(mario->GetBox());
		mario->SetFrame(0);
	});

	pipe_movement->SetOnAction([](Animator* animator, const Animation& anim) {
		Sprite_MoveAction(mario, (const MovingAnimation&)anim);
	});
	pipe_movement->SetOnStart([](Animator* animator) {
		mario->SetHasDirectMotion(true);
		mario->GetGravityHandler().setGravityAddicted(false);
		mario->GetGravityHandler().SetFalling(false);
		mario->SetFrame(0);
		if (mario->lastMovedRight)
			mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_right"));
		else
			mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".stand_left"));
		AnimatorManager::GetSingleton().MarkAsSuspended(walk);

		if (jump->IsAlive()) {
			jump->Stop();
		}
		disable_input = true;
	});
	
	walk->Start(new MovingAnimation("walk", 0, 0, 0, 80), GetGameTime());
}

string loadAllCharacters(const ALLEGRO_CONFIG* config) {

	return "Mario_small.walk_right:" + string(al_get_config_value(config, "Mario_small", "walk_right")) + '$'
		 + "Mario_small.walk_left:" + string(al_get_config_value(config, "Mario_small", "walk_left")) + '$'
		 + "Mario_small.stand_right:" + string(al_get_config_value(config, "Mario_small", "stand_right")) + '$'
		 + "Mario_small.stand_left:" + string(al_get_config_value(config, "Mario_small", "stand_left")) + '$'
		 + "Mario_small.jump_right:" + string(al_get_config_value(config, "Mario_small", "jump_right")) + '$'
		 + "Mario_small.jump_left:" + string(al_get_config_value(config, "Mario_small", "jump_left")) + '$'
		 + "Mario_small.winning_sequence:" + string(al_get_config_value(config, "Mario_small", "winning_sequence")) + '$'
		 + "Mario_big.walk_right:" + string(al_get_config_value(config, "Mario_big", "walk_right")) + '$'
		 + "Mario_big.walk_left:" + string(al_get_config_value(config, "Mario_big", "walk_left")) + '$'
		 + "Mario_big.stand_right:" + string(al_get_config_value(config, "Mario_big", "stand_right")) + '$'
		 + "Mario_big.stand_left:" + string(al_get_config_value(config, "Mario_big", "stand_left")) + '$'
		 + "Mario_big.jump_right:" + string(al_get_config_value(config, "Mario_big", "jump_right")) + '$'
		 + "Mario_big.jump_left:" + string(al_get_config_value(config, "Mario_big", "jump_left")) + '$'
		 + "Mario_big.winning_sequence:" + string(al_get_config_value(config, "Mario_big", "winning_sequence")) + '$'
		;
}

string loadAllPipes(const ALLEGRO_CONFIG* config) {

	return "Pipe.up:" + string(al_get_config_value(config, "pipes", "pipe_up")) + '$'
		+ "Pipe.right:" + string(al_get_config_value(config, "pipes", "pipe_right")) + '$'
		+ "Pipe.left:" + string(al_get_config_value(config, "pipes", "pipe_left")) + '$'
		;
}

string loadAllEnemies(const ALLEGRO_CONFIG* config) {

	return "enemies.goomba:" + string(al_get_config_value(config, "enemies", "goomba")) + '$'
		+ "enemies.goomba_smashed:" + string(al_get_config_value(config, "enemies", "goomba_smashed")) + '$'
		+ "enemies.green_koopa_troopa_right:" + string(al_get_config_value(config, "enemies", "green_koopa_troopa_right")) + '$'
		+ "enemies.green_koopa_troopa_left:" + string(al_get_config_value(config, "enemies", "green_koopa_troopa_left")) + '$'
		+ "enemies.green_koopa_troopa_shell:" + string(al_get_config_value(config, "enemies", "green_koopa_troopa_shell")) + '$'
		+ "enemies.red_koopa_troopa_right:" + string(al_get_config_value(config, "enemies", "red_koopa_troopa_right")) + '$'
		+ "enemies.red_koopa_troopa_left:" + string(al_get_config_value(config, "enemies", "red_koopa_troopa_left")) + '$'
		+ "enemies.red_koopa_troopa_shell:" + string(al_get_config_value(config, "enemies", "red_koopa_troopa_shell")) + '$'
		+ "enemies.piranha_plant:" + string(al_get_config_value(config, "enemies", "piranha_plant")) + '$'
		;
}

string loadAllBlocks(const ALLEGRO_CONFIG* config) {

	return "blocks.brick:" + string(al_get_config_value(config, "blocks", "brick")) + '$'
		+ "blocks.block:" + string(al_get_config_value(config, "blocks", "block")) + '$'
		+ "blocks.empty_block:" + string(al_get_config_value(config, "blocks", "empty_block")) + '$'
		+ "blocks.coin:" + string(al_get_config_value(config, "blocks", "coin")) + '$'
		+ "blocks.pole:" + string(al_get_config_value(config, "blocks", "pole")) + '$'
		+ "blocks.green_ball:" + string(al_get_config_value(config, "blocks", "green_ball")) + '$'
		;
}

string loadAllPowerups(const ALLEGRO_CONFIG* config) {

	return "powerups.super:" + string(al_get_config_value(config, "powerups", "super")) + '$'
		+ "powerups.1up:" + string(al_get_config_value(config, "powerups", "1up")) + '$'
		+ "powerups.star:" + string(al_get_config_value(config, "powerups", "star")) + '$'
		;
}


void app::MainApp::Load(void) {
	ALLEGRO_CONFIG* config = al_load_config_file(".\\Engine\\config.ini");
	InitialiseParams(config);
	game.setInitLifes(game_params.initial_player_lifes);
	assert(config != NULL);

	//load bitmaps, TODO we shouldnt have a bitmap loader at all. Animation film will handle this
	Bitmap tiles = bitmaploader->Load(al_get_config_value(config, "paths", "tiles_path"));
	Bitmap bg_tiles = bitmaploader->Load(al_get_config_value(config, "paths", "bg_tiles_path"));
	characters = bitmaploader->Load(al_get_config_value(config, "paths", "characters_path"));
	npcs = bitmaploader->Load(al_get_config_value(config, "paths", "npcs_path"));
	assert(npcs);

	action_layer = new TileLayer(MAX_HEIGHT, MAX_WIDTH, tiles);
	action_layer->Allocate();

	int tilesw = DIV_TILE_WIDTH(BitmapGetWidth(tiles)); //tileset width
	int tilesh = DIV_TILE_HEIGHT(BitmapGetHeight(tiles)); //tileset height
	total_tiles = tilesw * tilesh;

	action_layer->InitCaching(tilesw, tilesh);
	loadMap(action_layer, al_get_config_value(config, "paths", "action_layer_path"));

	underground_layer = new TileLayer(MAX_HEIGHT, MAX_WIDTH, tiles);
	underground_layer->Allocate();
	underground_layer->InitCaching(tilesw, tilesh);
	loadMap(underground_layer, al_get_config_value(config, "paths", "underground_layer_path"));

	circular_background = new CircularBackground(bg_tiles, al_get_config_value(config, "paths", "circular_backround_path"));
	menu_circular_background = new CircularBackground(bg_tiles, al_get_config_value(config, "paths", "menu_backround_path"));

	loadSolidTiles(config, action_layer);
	action_layer->ComputeTileGridBlocks1();

	AnimationFilmHolder::GetInstance().LoadAll(loadAllCharacters(config), al_get_config_value(config, "paths", "characters_path"));
	AnimationFilmHolder::GetInstance().LoadAll(loadAllPipes(config), al_get_config_value(config, "paths", "tiles_path"));
	AnimationFilmHolder::GetInstance().LoadAll(loadAllEnemies(config), al_get_config_value(config, "paths", "enemies_path"));
	AnimationFilmHolder::GetInstance().LoadAll(loadAllBlocks(config), al_get_config_value(config, "paths", "npcs_path"));
	AnimationFilmHolder::GetInstance().LoadAll(loadAllPowerups(config), al_get_config_value(config, "paths", "npcs_path"));

	liveIcon = SubBitmapCreate(BitmapLoad(al_get_config_value(config, "paths", "characters_path")), { 127, 60, 16, 16 });
	coinIcon = SubBitmapCreate(BitmapLoad(al_get_config_value(config, "paths", "npcs_path")), { 0, 16, 16, 16 });

	checkpoint_x = atoi(splitString(al_get_config_value(config, "potitions", "checkpoint"), " ")[0].c_str());

	vector<string> coordinates = splitString(al_get_config_value(config, "potitions", "start"), " ");
	mario = new Sprite(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()), AnimationFilmHolder::GetInstance().GetFilm("Mario_small.stand_right"), "mario");
	SpriteManager::GetSingleton().Add(mario);

	mario->SetMover([](const Rect& pos, int* dx, int* dy) {
		int old_dx = *dx;
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};
		if (gridOn)
			action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
		mario->SetPos(pos.x + *dx, pos.y + *dy);

		if (old_dx > * dx && old_dx > 0 || old_dx < *dx && old_dx < 0) {
			mario->SetStateId(IDLE_STATE);
		}
		});

	mario->SetBoundingArea(new BoundingBox(mario->GetBox().x, mario->GetBox().y, mario->GetBox().x + mario->GetBox().w, mario->GetBox().y + mario->GetBox().h));
	mario->SetFormStateId(SMALL_MARIO);
	mario->Set_Str_StateId("Mario_small");

	PrepareSpriteGravityHandler(action_layer->GetGrid(), mario);

	std::vector<std::string> enemies_names = { "goomba", "green_koopa_troopa", "red_koopa_troopa", "piranha_plant" };
	vector<string> locations;
	for (std::string enemie_name : enemies_names) {
		enemies_positions[enemie_name] = std::vector<std::string>();
		locations = splitString(al_get_config_value(config, "emenies_positions", enemie_name.c_str()), ",");
		for (auto location : locations)
			enemies_positions[enemie_name].push_back(location);
	}

	//create all pipe sprites and add collisions
	for (auto pipes : splitString(al_get_config_value(config, "pipes", "pipe_locations"), ",")) {

		Sprite* pipe = LoadPipeCollision(mario, pipes);
		pipe->SetFormStateId(PIPE);
		SpriteManager::GetSingleton().Add(pipe);

	}

	//create super mushroom
	locations = splitString(al_get_config_value(config, "powerups_positions", "super"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_super_mushroom(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()));
	}

	//create 1up mushroom
	locations = splitString(al_get_config_value(config, "powerups_positions", "1up"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_1UP_mushroom(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()), &game);
	}

	//create star
	locations = splitString(al_get_config_value(config, "powerups_positions", "star"), ",");
	for (auto location : locations) {
		coordinates = splitString(location, " ");
		create_starman(atoi(coordinates[0].c_str()), atoi(coordinates[1].c_str()));
	}

	for (unsigned int i = 0; i < action_layer->GetMapWidth(); i++) {
		for (unsigned int j = 0; j < action_layer->GetMapHeight(); j++) {
			// replace each brick index with sprite
			if (action_layer->GetTile(j, i) == 0) {
				create_block_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), &game);
			}
			else

				// replace each block index with sprite
				if (action_layer->GetTile(j, i) == 4) {
					create_brick_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j));
				}
				else

					// replace each coin index with sprite
					if (action_layer->GetTile(j, i) == 26) {
						create_coin_sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), &game);
					}
					else
						//create flag pole
						if (action_layer->GetTile(j, i) == 496 || action_layer->GetTile(j, i) == 470) { //this is the final flag
							Sprite* pole = nullptr;
							if (action_layer->GetTile(j, i) == 496)
								pole = new Sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), AnimationFilmHolder::GetInstance().GetFilm("blocks.pole"), "pole");
							else {
								end_x = MUL_TILE_WIDTH(i);
								pole = new Sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), AnimationFilmHolder::GetInstance().GetFilm("blocks.green_ball"), "pole");
							}
							SpriteManager::GetSingleton().Add(pole);
							pole->SetHasDirectMotion(true);
							pole->GetGravityHandler().setGravityAddicted(false);
							pole->SetBoundingArea(new BoundingBox(pole->GetBox().x, pole->GetBox().y, pole->GetBox().x + pole->GetBox().w, pole->GetBox().y + pole->GetBox().h));
							CollisionChecker::GetSingleton().Register(mario, pole, [this](Sprite* s1, Sprite* s2) {
								disable_input = true;
								mario->SetVisibility(true);
								CollisionChecker::GetSingleton().CancelAll(mario);
								if (mario->won) return;
								mario->won = true;
								AnimatorManager::GetSingleton().CancelAndRemoveAll();

								mario->SetFrame(0);
								mario->SetStateId(WALKING_STATE);
								mario->GetGravityHandler().setGravityAddicted(false);
								mario->GetGravityHandler().SetFalling(false);
								mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".winning_sequence"));

								MovingAnimator* finish_sequence = new MovingAnimator();
								mario->Move(7, 0);
								finish_sequence->SetOnAction([this](Animator* animator, const Animation& anim) {
									Sprite_MoveAction(mario, (const MovingAnimation&)anim);
									game.addPointsNoShow(10);
									if (mario->GetGravityHandler().isOnSolidGround(mario->GetBox())) {
										animator->Stop();
									}
									});

								finish_sequence->SetOnFinish([](Animator* animator) { //when we get here, mario is on the ground. stand walking right
									cout << "CONGRATZ!\n";
									//animator->deleteCurrAnimation();
									mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".walk_right"));
									mario->SetFrame(0);
									mario->GetGravityHandler().setGravityAddicted(true);

									//start walking right
									animator->SetOnAction([](Animator* animator, const Animation& anim) {
										Sprite_MoveAction(mario, (const MovingAnimation&)anim);
										});

									((MovingAnimator*)animator)->Start(new MovingAnimation("finish_sequence", 80, 2, 0, 20), GetGameTime());
									animator->SetOnFinish([](Animator* animator) {
										winFinished = true;
										mario->SetVisibility(false);
										AnimatorManager::GetSingleton().Cancel(animator);
										});
									});

								AnimatorManager::GetSingleton().Register(finish_sequence);
								finish_sequence->Start(new MovingAnimation("finish_sequence", 0, 0, 1, 20), GetGameTime());

								});
						}
			/*else if (action_layer->GetTile(j, i) == 471) {
				Sprite *small_flag = new Sprite(MUL_TILE_WIDTH(i), MUL_TILE_HEIGHT(j), AnimationFilmHolder::GetInstance().GetFilm("blocks.small_flag"), "small_flag");
				action_layer->SetTile(j, i, action_layer->GetTile(j, i - 1)); //set it equal to the tile above
				SpriteManager::GetSingleton().Add(small_flag);
				small_flag->SetHasDirectMotion(true);
				small_flag->GetGravityHandler().setGravityAddicted(false);
				small_flag->SetVisibility(false);
			}*/
		}
	}
}

void mainMenu() {
	al_flush_event_queue(queue);
	int total_scroll = 0;
	while(true){
		menu_circular_background->Display(al_get_backbuffer(display), displayArea.x, displayArea.y);

		al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 30, ALLEGRO_ALIGN_CENTER, "CSD4022 - ANTONIS PARAGIOUDAKIS");
		al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 70, ALLEGRO_ALIGN_CENTER, "CSD4101 - MIXALIS RAPTAKIS");
		al_draw_text(tittle_font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 110, ALLEGRO_ALIGN_CENTER, "CSD4017 - GEORGOS PANTELAKIS");
		al_draw_text(tittle_font_smaller, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 220, ALLEGRO_ALIGN_CENTER, "University of Crete");
		al_draw_text(tittle_font_smaller, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 250, ALLEGRO_ALIGN_CENTER, "Department of Computer Science");
		al_draw_text(tittle_font_smaller, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 280, ALLEGRO_ALIGN_CENTER, "CS-454. Development of Intelligent Interfaces and Games");
		al_draw_text(tittle_font_smaller, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 310, ALLEGRO_ALIGN_CENTER, "Term Project, Fall Semester 2020");
		al_draw_text(font, al_map_rgb(0, 0, 0), action_layer->GetViewWindow().w / 2, 420, ALLEGRO_ALIGN_CENTER, "Press ENTER or SPACE to continue...");


		al_flip_display();

		if (!al_is_event_queue_empty(queue)) {
			al_wait_for_event(queue, &event);
			if (event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_ENTER
				|| event.type == ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode == ALLEGRO_KEY_SPACE) {
				delete menu_circular_background;
				break;
			}
			else if (event.type == ALLEGRO_EVENT_TIMER) {
				menu_circular_background->Scroll(1);
				total_scroll++;
			}
			else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
				closeWindowClicked = true;
				break;
			}
		}
	}
}

void app::App::Run(void) {
	mainMenu();
	al_flush_event_queue(fallingQueue);
	al_flush_event_queue(aiQueue);
	al_flush_event_queue(showQueue);
	game.MainLoop();
}

void app::MainApp::Clear(void) {
	al_destroy_font(font);
	al_destroy_font(paused_font);
	al_destroy_font(tittle_font);
	al_destroy_font(tittle_font_smaller);
	al_destroy_display(display);
	al_uninstall_keyboard();
	al_uninstall_mouse();
	al_destroy_bitmap(action_layer->GetBitmap());
	al_destroy_bitmap(underground_layer->GetBitmap());
	al_destroy_sample(jumpEffect);
	al_destroy_sample(deathEffect);
	al_destroy_sample(bgSong);
	al_destroy_sample_instance(backgroundSong);
	//TODO destroy grid, tiles, background
	delete action_layer;
	delete underground_layer;
	delete bitmaploader;
}

void app::App::Main(void) {
	Initialise();
	Load();
	Run();
	Clear();
}

/*--------------------FUNCTIONS-------------------------*/

bool app::ReadTextMap(class TileLayer* layer, string filename) {
	string line, token, delimiter = ",";
	size_t pos = 0;
	ifstream csvFile(filename);
	int x = 0, y = 0;

	if (csvFile.is_open()) {
		while (getline(csvFile, line)) {
			x = 0;
			while ((pos = line.find(delimiter)) != string::npos) {
				token = line.substr(0, pos);
				stringstream ss(token);
				int val;
				ss >> val;
				if (val == -1) {
					layer->SetTile(x, y, total_tiles); //"create" an extra tile which is going to be identified as the trasnparent tile
				}
				else {
					layer->SetTile(x, y, val);
				}

				x++;
				line.erase(0, pos + delimiter.length());
			}
			stringstream ss(line);
			int val;
			ss >> val;
			layer->SetTile(x, y, val);
			x++;
			y++;
		}
		layer->SetMapDims(x, y);
		widthInTiles = x;
		heightInTiles = y;
		csvFile.close();
		return true;
	}
	else {
		cout << "Unable to open file " << filename << endl;
		exit(EXIT_FAILURE);
	}

	return false;
}

Index GetCol(Index index)
{
	return index >> 4;
}
Index GetRow(Index index)
{
	return index & 0xF0;
}

int app::GetMapPixelWidth(void) {
	return widthInTiles * TILE_WIDTH > displayArea.w ? widthInTiles * TILE_WIDTH : displayArea.w;
}

int app::GetMapPixelHeight(void) {
	return heightInTiles * TILE_HEIGHT > displayArea.h ? heightInTiles * TILE_HEIGHT : displayArea.h;
}

bool app::characterStaysInCenter(Rect pos, int* dx) {
	return pos.x + pos.w/2 - *dx > displayArea.w/2;
}

void app::Game::Pause(uint64_t t) {
	isPaused = true; pauseTime = t; Invoke(pauseResume);
}

void app::Game::Resume(void) {
	isPaused = false; Invoke(pauseResume); pauseTime = 0;
}

bool app::Game::IsPaused(void) const {
	return isPaused;
}

uint64_t app::Game::GetPauseTime(void) const {
	return pauseTime;
}

int app::Game::getLives(void) {
	return lives;
}

void app::Game::addLife(void) {
	lives++;
}

void app::Game::loseLife(void) {
	lives--;
}

bool app::Game::isGameOver(void) {
	return lives == 0;
}

int app::Game::getCoins(void) {
	return coins;
}

void app::Game::addCoin(void) {
	coins += 1;

	if (coins == game_params.coins_to_get_life) {
		resetCoins();
		addLife();
	}
}

void app::Game::resetCoins(void) {
	coins = 0;
}

int app::Game::getPoints(void) {
	return points;
}

void app::Game::addPoints(int extra_points) {
	struct pointShow* tmp = new pointShow({ extra_points, mario->GetBox().x, mario->GetBox().y, mario->GetBox().y - 50 });
	pointsShowList.push_back(tmp);
	points += extra_points;
}

void app::Game::addPointsNoShow(int extra_points) {
	points += extra_points;
}