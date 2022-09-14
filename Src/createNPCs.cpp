#include "../Include/createNPCs.h"
#include "../Include/app.h"
#include "../Include/Utilities.h"

using namespace std;

extern TileLayer* action_layer;
extern TileLayer* underground_layer;
extern CircularBackground* circular_background;
extern Sprite* mario;
extern FrameRangeAnimator* jump;
extern FrameRangeAnimation* jump_anim;
extern std::unordered_set<Sprite*> shells;
extern MovingAnimator* walk, * pipe_movement;
extern bool keys[ALLEGRO_KEY_MAX];
extern bool disable_input;
extern ALLEGRO_TIMER* blockTimer;
extern struct game_params game_params;

void collisionBlockWithEnemies(Sprite* block, Sprite* enemy) {
	CollisionChecker::GetSingleton().Register(block, enemy,
		[](Sprite* s1, Sprite* s2) {
			if (s2->GetFormStateId() == SMASHED)
				return;
			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();

			if (s1_y1 < s2_y2) {
				if (s1->GetFormStateId() == MOVED_BLOCK || s1->GetFormStateId() == SMASHED) {
					//kill animation

					s2->SetFormStateId(DELETE_BY_BLOCK);
				}
			}
		}
	);
}

void collisionBlockWithPowerUps(Sprite* block, Sprite* powerUp) {
	CollisionChecker::GetSingleton().Register(block, powerUp,
		[](Sprite* s1, Sprite* s2) {
			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();

			if (s1_y1 < s2_y2) {
				s2->lastMovedRight = !s2->lastMovedRight;
			}
		}
	);
}

//create enemies
Sprite* app::create_enemy_goomba(int x, int y) {
	Sprite* goomba = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("enemies.goomba"), "goomba");
	SpriteManager::GetSingleton().Add(goomba);

	MovingAnimator* goomba_walk = new MovingAnimator();
	goomba->SetAnimator(goomba_walk);
	AnimatorManager::GetSingleton().Register(goomba_walk);
	//goomba_walk->
	goomba_walk->SetOnAction([goomba](Animator* animator, const Animation& anim) {
		goomba->NextFrame();
	});

	MovingAnimation* goomba_walking_animation = new MovingAnimation("goomba_walk", 0, 0, 0, 100);
	goomba_walk->Start(goomba_walking_animation, GetGameTime());
	goomba->SetStateId(WALKING_STATE);
	goomba->SetZorder(1);
	goomba->SetBoundingArea(new BoundingBox(goomba->GetBox().x, goomba->GetBox().y, goomba->GetBox().x + goomba->GetBox().w, goomba->GetBox().y + goomba->GetBox().h));
	goomba->GetGravityHandler().setGravityAddicted(true);
	goomba->SetFormStateId(ENEMY);

	goomba->GetGravityHandler().SetOnSolidGround([goomba](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, goomba->GetStateId());
	});

	goomba->SetMover([goomba](const Rect& pos, int* dx, int* dy) {
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};
		action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
		if (*dx == 0) {
			goomba->lastMovedRight = !goomba->lastMovedRight;
		}
		goomba->SetPos(pos.x + *dx, pos.y + *dy);
	});

	CollisionChecker::GetSingleton().Register(mario, goomba,
		[goomba_walk, goomba_walking_animation](Sprite* s1, Sprite* s2) {

			if (s1->GetFormStateId() == INVINCIBLE_MARIO_SMALL || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
				//if mario is invinsible, don't think about it. Just kill him
				delete goomba_walking_animation;
				goomba_walk->Stop();
				AnimatorManager::GetSingleton().Cancel(goomba_walk);
				CollisionChecker::GetSingleton().Cancel(s1, s2);
				s2->SetFormStateId(DELETE);
				goomba_walk->Destroy();
				return;
			}

			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if (s1_x2 >= s2_x1 && s1_x1 < s2_x2 && s1_y2 <= 6 + s2_y1) { //hits goomba from top
				s2->SetFormStateId(SMASHED);

				delete goomba_walking_animation;

				//jumping animation
				if (jump_anim != nullptr) {
					jump->Stop();
					delete jump_anim;
				}

				CollisionChecker::GetSingleton().Cancel(s1, s2);
				jump_anim = new FrameRangeAnimation("jump", 0, 17, 1, 0, -16, 15); //start, end, reps, dx, dy, delay
				jump_anim->SetChangeSpeed([](int& dx, int& dy, int frameNo) {
					int sumOfNumbers = jump_anim->GetEndFrame() * ((jump_anim->GetEndFrame() - 1) / 2);
					dy = -customRound((float)((jump_anim->GetEndFrame() - frameNo) * 3 * TILE_HEIGHT) / sumOfNumbers);
				});

				/*play death animation of goomba*/
				MovingAnimation* goomba_death_animation = new MovingAnimation("goomba_smash", 1, 0, 0, 750);
				s2->SetFrame(0);
				s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.goomba_smashed"));
				s2->SetHasDirectMotion(true);
				s2->Move(0, 9); //smashed is smaller so move him back to the ground
				goomba_walk->Start(goomba_death_animation, GetGameTime());
				goomba_walk->SetOnFinish([goomba_walk, s2](Animator* animator) {
					AnimatorManager::GetSingleton().Cancel(animator);
					for (auto s1 : shells)
						CollisionChecker::GetSingleton().Cancel(s1, s2);
					goomba_walk->deleteCurrAnimation();
					s2->SetFormStateId(DELETE);
					goomba_walk->Destroy();
				});

				jump->Start(jump_anim, GetGameTime());
				mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() +
					(mario->lastMovedRight ? ".jump_right" : ".jump_left")));
			}
			else {
				//do mario penalty
				mario->hit();
			}
		}
	);

	std::vector<std::string> block_types = { "brick", "block" };
	for (std::string block_type : block_types) {
		for (auto block : SpriteManager::GetSingleton().GetTypeList(block_type)) {
			collisionBlockWithEnemies(block, goomba);
		}
	}

	return goomba;
}

Sprite* app::create_enemy_green_koopa_troopa(int x, int y) {
	Sprite* koopa_troopa = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_left"), "green_koopa_troopa");
	SpriteManager::GetSingleton().Add(koopa_troopa);

	MovingAnimator* koopa_troopa_walk = new MovingAnimator();
	AnimatorManager::GetSingleton().Register(koopa_troopa_walk);

	koopa_troopa_walk->SetOnAction([koopa_troopa](Animator* animator, const Animation& anim) {
		koopa_troopa->NextFrame();
	});
	koopa_troopa->SetAnimator(koopa_troopa_walk);

	MovingAnimation* koopa_troopa_walking_animation = new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100);
	koopa_troopa_walk->Start(koopa_troopa_walking_animation, GetGameTime());
	koopa_troopa->SetStateId(WALKING_STATE);
	koopa_troopa->SetZorder(1);
	koopa_troopa->SetBoundingArea(new BoundingBox(koopa_troopa->GetBox().x, koopa_troopa->GetBox().y, koopa_troopa->GetBox().x + koopa_troopa->GetBox().w, koopa_troopa->GetBox().y + koopa_troopa->GetBox().h));
	koopa_troopa->GetGravityHandler().setGravityAddicted(true);
	koopa_troopa->SetFormStateId(ENEMY);

	koopa_troopa->GetGravityHandler().SetOnSolidGround([koopa_troopa](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, koopa_troopa->GetStateId());
	});

	koopa_troopa->SetMover([koopa_troopa](const Rect& pos, int* dx, int* dy) {
		int prev_x = *dx;
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};

		action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
		if (prev_x > * dx && prev_x > 0 || prev_x < *dx && prev_x < 0) {
			koopa_troopa->lastMovedRight = !koopa_troopa->lastMovedRight;
			if (koopa_troopa->GetFormStateId() != SMASHED) {
				if (koopa_troopa->lastMovedRight)
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_right"));
				else
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_left"));
			}
		}
		koopa_troopa->SetPos(pos.x + *dx, pos.y + *dy);
	});

	CollisionChecker::GetSingleton().Register(mario, koopa_troopa,
		[koopa_troopa_walk, koopa_troopa_walking_animation](Sprite* s1, Sprite* s2) {

			if (s1->GetFormStateId() == INVINCIBLE_MARIO_SMALL || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
				//if mario is invinsible, don't think about it. Just kill him
				if (s2->GetFormStateId() == SMASHED) {
					shells.erase(s2);
					if (s2->GetStateId() == IDLE_STATE) { //if he is not moving, he has an animation playing.
						koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
						koopa_troopa_walk->deleteCurrAnimation();
						
					}
				}
				koopa_troopa_walk->Stop();
				AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
				CollisionChecker::GetSingleton().CancelAll(s2);
				s2->SetFormStateId(DELETE);
				koopa_troopa_walk->Destroy();
				return;
			}

			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if (s1_x2 >= s2_x1 && s1_x1 < s2_x2 && s1_y2 <= 3 + s2_y1) { //hits  coopa/shell from top

				if (s2->GetFormStateId() == ENEMY) { // if alive and moving around
					s2->SetFrame(0);
					s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_shell"));
					s2->SetFormStateId(SMASHED);
					s2->SetStateId(IDLE_STATE);
					s2->SetBoxDimentions(16, 16);
					s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
					s2->Move(0, 4);

					auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
					sprites.remove(s2); //remove my self
					//HERE ADD MARIO COLLISION
					//CollisionChecker::GetSingleton().Register(s2, mario, [](Sprite* s1, Sprite* s2) {
						//TODO
					//});
					for (auto sprite : sprites) { //add shell collision with all sprites
						CollisionChecker::GetSingleton().Register(s2, sprite,
							[](Sprite* s1, Sprite* s2) {
								if (s1->GetFormStateId() == SMASHED && s2->GetFormStateId() == SMASHED) //2 sells colide? do nothing
									return;
								if (s1->GetStateId() == IDLE_STATE) //if someone collides when shell is not moving do nothing
									return;
								if (s2->GetTypeId() == "piranha_plant" && ((MovingPathAnimation*)s2->GetAnimator()->GetAnim())->GetPath().size() == 1 + ((MovingPathAnimator*)s2->GetAnimator())->GetFrame())
									return; //if piranha is under the pipe do nothing
								s2->SetFormStateId(DELETE);
								for (auto shell : shells)
									CollisionChecker::GetSingleton().Cancel(shell, s2);
								//CollisionChecker::GetSingleton().Cancel(s1, s2);
								CollisionChecker::GetSingleton().Cancel(mario, s2); //remove mario collision with other sprite
								Animator* tmp = s2->GetAnimator();
								tmp->deleteCurrAnimation();
								tmp->Stop();
								AnimatorManager::GetSingleton().Cancel(tmp);
								tmp->Destroy();
							}
						);
					}
					shells.insert(s2);

					koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_shell", 1, 0, 0, game_params.koopa_troopa_stun_duration * 1000), GetGameTime());
					koopa_troopa_walk->SetOnFinish([s2, koopa_troopa_walk](Animator* animator) { //transforms from shell to koopa troopa
						auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
						sprites.remove(s2); //remove my self
						//CollisionChecker::GetSingleton().Cancel(s2, mario); //remove shell collision with mario
						for (auto sprite : sprites) { //remove shell collision with sprites
							CollisionChecker::GetSingleton().Cancel(s2, sprite);
						}
						shells.erase(s2);
						if (s2->lastMovedRight)
							s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_right"));
						else
							s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_left"));

						s2->SetBoxDimentions(16, 22);
						s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
						s2->SetHasDirectMotion(true);
						s2->Move(0, -6);
						s2->SetHasDirectMotion(false);
						koopa_troopa_walk->deleteCurrAnimation();
						koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100), GetGameTime());
						koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
						s2->SetFormStateId(ENEMY);
						s2->SetStateId(WALKING_STATE);
						});

				}
				else { //mario hits the shell from the top
					mario->Move(0, -4); //move him just a little away from the shell so they don't collide again

					if (s2->GetStateId() == IDLE_STATE) { //shell is not moving. Start it moving
						//-->shell start moving
						koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
						koopa_troopa_walk->Stop();
						koopa_troopa_walk->deleteCurrAnimation();
						AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
						s2->SetStateId(WALKING_STATE);
						if (s1_x1 < s2_x1)
							s2->lastMovedRight = true;
						else
							s2->lastMovedRight = false;
					}
					else { // shell is already moving. Stop it

						koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_shell", 1, 0, 0, game_params.koopa_troopa_stun_duration *1000), GetGameTime());
						koopa_troopa_walk->SetOnFinish([s2, koopa_troopa_walk](Animator* animator) {
							auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
							sprites.remove(s2); //remove my self
							//CollisionChecker::GetSingleton().Cancel(s2, mario); //remove shell collision with mario
							for (auto sprite : sprites) { //remove shell collision with sprites
								CollisionChecker::GetSingleton().Cancel(s2, sprite);
							}
							shells.erase(s2);
							if (s2->lastMovedRight)
								s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_right"));
							else
								s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.green_koopa_troopa_left"));

							s2->SetBoxDimentions(16, 22);
							s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
							s2->SetHasDirectMotion(true);
							s2->Move(0, -6);
							s2->SetHasDirectMotion(false);
							koopa_troopa_walk->deleteCurrAnimation();
							koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100), GetGameTime());
							koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
							s2->SetFormStateId(ENEMY);
							s2->SetStateId(WALKING_STATE);
							});
						s2->SetFrame(0);
						s2->SetStateId(IDLE_STATE);
					}
				}

				//memory cleaning
				if (jump_anim != nullptr) {
					jump->Stop();
					delete jump_anim;
				}

				/*mario jumping animation after hitting koopa*/
				jump_anim = new FrameRangeAnimation("jump", 0, 17, 1, 0, -16, 15); //start, end, reps, dx, dy, delay

				jump_anim->SetChangeSpeed([](int& dx, int& dy, int frameNo) {
					int sumOfNumbers = 0;
					char maxTiles = 3;

					for (int i = 1; i <= jump_anim->GetEndFrame(); i++) sumOfNumbers += i;

					dy = -customRound((float)((jump_anim->GetEndFrame() - frameNo) * maxTiles * TILE_HEIGHT) / sumOfNumbers);
					});
				jump->Start(jump_anim, GetGameTime());
				if (mario->lastMovedRight)
					mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_right"));
				else
					mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_left"));
			}
			else { //mario hits shell from right or left

				if (s2->GetFormStateId() == SMASHED && s2->GetStateId() == IDLE_STATE) { //shell starts moving
					koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
					koopa_troopa_walk->Stop();
					koopa_troopa_walk->deleteCurrAnimation();
					AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
					if (s1_x1 < s2_x1) {
						s2->lastMovedRight = true;
						s2->Move(16, 0);
					}
					else {
						s2->lastMovedRight = false;
						s2->Move(-16, 0);
					}
					s2->SetStateId(WALKING_STATE);
				}
				else {
					//do mario penalty
					if (!(s2->GetFormStateId() == SMASHED && 
						(mario->lastMovedRight == true && s1_x1 < s2_x1 || mario->lastMovedRight == false && s2_x2 < s1_x2)))
					{ //do not hit mario when he is pushing the shell
						mario->hit();
					}
				}
			}
		}
	);

	std::vector<std::string> block_types = { "brick", "block" };
	for (std::string block_type : block_types) {
		for (auto block : SpriteManager::GetSingleton().GetTypeList(block_type)) {
			collisionBlockWithEnemies(block, koopa_troopa);
		}
	}

	return koopa_troopa;
}

Sprite* app::create_enemy_red_koopa_troopa(int x, int y) {
	Sprite* koopa_troopa = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_left"), "red_koopa_troopa");
	SpriteManager::GetSingleton().Add(koopa_troopa);

	MovingAnimator* koopa_troopa_walk = new MovingAnimator();
	AnimatorManager::GetSingleton().Register(koopa_troopa_walk);

	koopa_troopa_walk->SetOnAction([koopa_troopa](Animator* animator, const Animation& anim) {
		koopa_troopa->NextFrame();
	});
	koopa_troopa->SetAnimator(koopa_troopa_walk);

	MovingAnimation* koopa_troopa_walking_animation = new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100);
	koopa_troopa_walk->Start(koopa_troopa_walking_animation, GetGameTime());
	koopa_troopa->SetStateId(WALKING_STATE);
	koopa_troopa->SetZorder(1);
	koopa_troopa->SetBoundingArea(new BoundingBox(koopa_troopa->GetBox().x, koopa_troopa->GetBox().y, koopa_troopa->GetBox().x + koopa_troopa->GetBox().w, koopa_troopa->GetBox().y + koopa_troopa->GetBox().h));
	koopa_troopa->GetGravityHandler().setGravityAddicted(true);
	koopa_troopa->SetFormStateId(ENEMY);

	koopa_troopa->GetGravityHandler().SetOnSolidGround([koopa_troopa](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, koopa_troopa->GetStateId());
	});

	koopa_troopa->SetMover([koopa_troopa](const Rect& pos, int* dx, int* dy) {
		int prev_x = *dx;
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};

		action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);

		if (prev_x > * dx && prev_x > 0 || prev_x < *dx && prev_x < 0) { //maybe hit a wall/pipe
			koopa_troopa->lastMovedRight = !koopa_troopa->lastMovedRight;
			if (koopa_troopa->GetFormStateId() == ENEMY) {
				if (koopa_troopa->lastMovedRight)
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_right"));
				else
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_left"));
			}
		}
		else {
			if (!action_layer->GetGrid()->IsOnSolidGround(posOnGrid, WALKING_STATE) && koopa_troopa->GetFormStateId() == ENEMY) {
				if (koopa_troopa->lastMovedRight) {
					*dx = (-1) * (int)game_params.enemy_speed;
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_left"));
				}
				else {
					*dx = game_params.enemy_speed;
					koopa_troopa->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_right"));
				}
				*dy = 0;
				koopa_troopa->lastMovedRight = !koopa_troopa->lastMovedRight;
			}

		}
		koopa_troopa->SetPos(pos.x + *dx, pos.y + *dy);
	});

	CollisionChecker::GetSingleton().Register(mario, koopa_troopa,
		[koopa_troopa_walk](Sprite* s1, Sprite* s2) {

			if (s1->GetFormStateId() == INVINCIBLE_MARIO_SMALL || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
				//if mario is invinsible, don't think about it. Just kill him
				if (s2->GetFormStateId() == SMASHED) {
					shells.erase(s2);
					if (s2->GetStateId() == IDLE_STATE) { //if he is not moving, he has an animation playing.
						koopa_troopa_walk->SetOnFinish([](Animator * animator) {});
						koopa_troopa_walk->deleteCurrAnimation();
					}
				}
				koopa_troopa_walk->Stop();
				AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
				CollisionChecker::GetSingleton().CancelAll(s2);
				s2->SetFormStateId(DELETE);
				koopa_troopa_walk->Destroy();
				return;
			}

			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if (s1_x2 >= s2_x1 && s1_x1 < s2_x2 && s1_y2 <= 3 + s2_y1) { //mario hits  coopa/shell from top
				
				if (s2->GetFormStateId() == ENEMY) { // if alive and moving around
					s2->SetFrame(0);
					s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_shell"));
					s2->SetFormStateId(SMASHED);
					s2->SetStateId(IDLE_STATE);
					s2->SetBoxDimentions(16, 16);
					s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
					s2->Move(0, 4);
					auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
					for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
					sprites.remove(s2); //remove my self
					//HERE ADD MARIO COLLISION
					//CollisionChecker::GetSingleton().Register(s2, mario, [](Sprite* s1, Sprite* s2) {
					//	//TODO
					//});

					for (auto sprite : sprites) { //add shell collision with all sprites
						CollisionChecker::GetSingleton().Register(s2, sprite,
							[](Sprite* s1, Sprite* s2) {
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

								//SpriteManager::GetSingleton().Remove(s2); //bye bye
							}
						);
					}
					shells.insert(s2);

					koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_shell", 1, 0, 0, game_params.koopa_troopa_stun_duration*1000), GetGameTime());
					koopa_troopa_walk->SetOnFinish([s2, koopa_troopa_walk](Animator* animator) {
						auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
						for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
						sprites.remove(s2); //remove my self
						//CollisionChecker::GetSingleton().Cancel(s2, mario); //remove shell collision with mario
						//CollisionChecker::GetSingleton().CancelAll(s2);
						for (auto sprite : sprites) { //remove shell collision with sprites
							CollisionChecker::GetSingleton().Cancel(s2, sprite);
						}
						shells.erase(s2);
						if (s2->lastMovedRight)
							s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_right"));
						else
							s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_left"));

						s2->SetBoxDimentions(16, 22);
						s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
						s2->SetHasDirectMotion(true);
						s2->Move(0, -6);
						s2->SetHasDirectMotion(false);
						koopa_troopa_walk->deleteCurrAnimation();
						koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100), GetGameTime());
						koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
							s2->SetFormStateId(ENEMY);
							s2->SetStateId(WALKING_STATE);
						});

				}
				else { //mario hits the shell from the top
					mario->Move(0, -4); //move him just a little away from the shell so they don't collide again

					if (s2->GetStateId() == IDLE_STATE) { //shell is not moving. Start it moving
						//-->shell start moving
						koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
						koopa_troopa_walk->Stop();
						koopa_troopa_walk->deleteCurrAnimation();
						AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
						s2->SetStateId(WALKING_STATE);
						if (s1_x1 < s2_x1)
							s2->lastMovedRight = true;
						else
							s2->lastMovedRight = false;
					}
					else { // shell is already moving. Stop it

						koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_shell", 1, 0, 0, game_params.koopa_troopa_stun_duration * 1000), GetGameTime());
						koopa_troopa_walk->SetOnFinish([s2, koopa_troopa_walk](Animator* animator) {
							auto sprites = SpriteManager::GetSingleton().GetTypeList("goomba");
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa")) sprites.push_back(sprite);
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa")) sprites.push_back(sprite);
							for (auto sprite : SpriteManager::GetSingleton().GetTypeList("piranha_plant")) sprites.push_back(sprite);
							sprites.remove(s2); //remove my self
							//CollisionChecker::GetSingleton().Cancel(s2, mario); //remove shell collision with mario
							for (auto sprite : sprites) { //remove shell collision with sprites
								CollisionChecker::GetSingleton().Cancel(s2, sprite);
							}
							shells.erase(s2);
							if (s2->lastMovedRight)
								s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_right"));
							else
								s2->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm("enemies.red_koopa_troopa_left"));

							s2->SetBoxDimentions(16, 22);
							s2->ReplaceBoundingArea(new BoundingBox(s2->GetBox().x, s2->GetBox().y, s2->GetBox().x + s2->GetBox().w, s2->GetBox().y + s2->GetBox().h));
							s2->SetHasDirectMotion(true);
							s2->Move(0, -6);
							s2->SetHasDirectMotion(false);
							koopa_troopa_walk->deleteCurrAnimation();
							koopa_troopa_walk->Start(new MovingAnimation("koopa_troopa_walk", 0, 0, 0, 100), GetGameTime());
							koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
							s2->SetFormStateId(ENEMY);
							s2->SetStateId(WALKING_STATE);
							});
						s2->SetFrame(0);
						s2->SetStateId(IDLE_STATE);
					}
				}

				//memory cleaning
				if (jump_anim != nullptr) {
					jump->Stop();
					delete jump_anim;
				}

				/*mario jumping animation after hitting koopa*/
				jump_anim = new FrameRangeAnimation("jump", 0, 17, 1, 0, -16, 15); //start, end, reps, dx, dy, delay

				jump_anim->SetChangeSpeed([](int& dx, int& dy, int frameNo) {
					int sumOfNumbers = 0;
					char maxTiles = 3;

					for (int i = 1; i <= jump_anim->GetEndFrame(); i++) sumOfNumbers += i;

					dy = -customRound((float)((jump_anim->GetEndFrame() - frameNo) * maxTiles * TILE_HEIGHT) / sumOfNumbers);
					});
				jump->Start(jump_anim, GetGameTime());
				if (mario->lastMovedRight)
					mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_right"));
				else
					mario->SetCurrFilm(AnimationFilmHolder::GetInstance().GetFilm(mario->Get_Str_StateId() + ".jump_left"));
			}
			else { //mario hits coopa/shell from right or left
			
				if (s2->GetFormStateId() == SMASHED && s2->GetStateId() == IDLE_STATE) { //shell is not moving. Start it
					
					koopa_troopa_walk->SetOnFinish([](Animator* animator) {});
					koopa_troopa_walk->Stop();
					koopa_troopa_walk->deleteCurrAnimation();
					AnimatorManager::GetSingleton().Cancel(koopa_troopa_walk);
					if (s1_x1 < s2_x1) {
						s2->lastMovedRight = true;
						s2->Move(16, 0);
					}
					else {
						s2->lastMovedRight = false;
						s2->Move(-16, 0);
					}
					s2->SetStateId(WALKING_STATE);
				}
				else {
					//do mario penalty
					if (!(s2->GetFormStateId() == SMASHED && 
						(mario->lastMovedRight  == true && s1_x1 < s2_x1 || mario->lastMovedRight == false && s2_x2 < s1_x2))) 
					{ //do not hit mario when he is pushing the shell
						mario->hit();
					}
				}
			}
		}
	);

	std::vector<std::string> block_types = { "brick", "block" };
	for (std::string block_type : block_types) {
		for (auto block : SpriteManager::GetSingleton().GetTypeList(block_type)) {
			collisionBlockWithEnemies(block, koopa_troopa);
		}
	}

	return koopa_troopa;
}

Sprite* app::create_enemy_piranha_plant(int x, int y) {
	Sprite* piranha = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("enemies.piranha_plant"), "piranha_plant");
	SpriteManager::GetSingleton().Add(piranha);

	class MovingPathAnimator* piranha_move = new MovingPathAnimator();
	piranha->SetAnimator(piranha_move);
	AnimatorManager::GetSingleton().Register(piranha_move);
	piranha_move->SetOnAction([piranha_move, piranha](Animator* animator, const Animation& anim) {
		int dx = ((const MovingPathAnimation&)anim).GetPath().at(piranha_move->GetFrame()).dx;
		int dy = ((const MovingPathAnimation&)anim).GetPath().at(piranha_move->GetFrame()).dy;
		piranha->Move(dx, dy);

		piranha_move->nextFrame(); //moves him up/down
		if (piranha_move->GetFrame() % 4 == 0)
			piranha->NextFrame();	// opens/closes mouth
		});

	MovingPathAnimation::Path path;
	for (int i = 0; i < 12; i++) {	//plant goes up
		path.push_back(struct PathEntry());
		path.at(i).delay = 70;
		path.at(i).dy = -2;
	}
	for (int i = 12; i < 40; i++) { //plant does nothing
		path.push_back(struct PathEntry());
		path.at(i).delay = 80;
	}
	for (int i = 40; i < 52; i++) { //plant goes down
		path.push_back(struct PathEntry());
		path.at(i).delay = 70;
		path.at(i).dy = 2;
	}
	path.push_back(struct PathEntry());
	path.at(52).delay = 2000; //do nothing while bellow the pipe
	//last frame is also used as "idle" state. in a lot of parts we check if we are at the last frame so be carefull when modifing!

	class MovingPathAnimation* piranha_moving_animation = new MovingPathAnimation("piranha", path);
	piranha_move->Start(piranha_moving_animation, GetGameTime());

	piranha->SetStateId(WALKING_STATE);
	piranha->SetZorder(1);
	piranha->SetBoundingArea(new BoundingBox(piranha->GetBox().x, piranha->GetBox().y, piranha->GetBox().x + piranha->GetBox().w, piranha->GetBox().y + piranha->GetBox().h));
	piranha->GetGravityHandler().setGravityAddicted(false);
	piranha->SetFormStateId(ENEMY);
	piranha->SetHasDirectMotion(true);

	CollisionChecker::GetSingleton().Register(mario, piranha,
		[piranha_move](Sprite* s1, Sprite* s2) {

			
			if (s1->GetFormStateId() == INVINCIBLE_MARIO_SMALL || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
				//if mario is invinsible, don't think about it. Just kill him
				if (piranha_move->GetFrame() + 1 == ((MovingPathAnimation*)(piranha_move->GetAnim()))->GetPath().size()) {//piranha is bellow the pipe dont kill him
					piranha_move->SetLastTime(GetGameTime());
					return;
				}
				piranha_move->deleteCurrAnimation();
				piranha_move->Stop();
				AnimatorManager::GetSingleton().Cancel(piranha_move);
				CollisionChecker::GetSingleton().Cancel(s1, s2);
				s2->SetFormStateId(DELETE);
				piranha_move->Destroy();
				return;
			}	

			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if (piranha_move->GetFrame() + 1 == ((MovingPathAnimation*)(piranha_move->GetAnim()))->GetPath().size()) { //piranha is bellow the pipe
				if (s1_x2 >= s2_x1 && s1_x1 < s2_x2 && s1_y2 <= 2 + s2_y1) { //mario on top of the piranha
					piranha_move->SetLastTime(GetGameTime());
				}
			}
			else { //mario AND piranha is on top of the pipe, mario gets hit
				//do mario penalty
				mario->hit();
			}

		}
	);
	return piranha;
}

//create blocks

void app::create_brick_sprite(int x, int y) {
	Sprite* brick = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("blocks.brick"), "brick");
	SpriteManager::GetSingleton().Add(brick);

	brick->SetHasDirectMotion(true);
	brick->SetStateId(IDLE_STATE);
	brick->SetZorder(3);
	brick->SetBoundingArea(new BoundingBox(brick->GetBox().x, brick->GetBox().y, brick->GetBox().x + brick->GetBox().w, brick->GetBox().y + brick->GetBox().h));
	brick->GetGravityHandler().setGravityAddicted(false);
	brick->SetFormStateId(BRICK);

	CollisionChecker::GetSingleton().Register(mario, brick,
		[](Sprite* s1, Sprite* s2) {
			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if (s1_x1 > s2_x1 - 12 && s1_x2 < s2_x2 + 12 && s1_y1 + 3 >= s2_y2) {
				s2->Move(0, -4);
				s2->SetFormStateId(MOVED_BLOCK);
				jump->Stop();
				if (jump_anim != nullptr)
					jump->deleteCurrAnimation();
				al_start_timer(blockTimer);
				if (s1->GetFormStateId() == SUPER_MARIO || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
					//smash animation

					s2->SetFormStateId(SMASHED);
				}
			}
		}
	);
}

void app::create_block_sprite(int x, int y, Game *game) {
	Sprite* block = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("blocks.block"), "block");
	SpriteManager::GetSingleton().Add(block);

	block->SetHasDirectMotion(true);
	block->SetStateId(IDLE_STATE);
	block->SetZorder(3);
	block->SetBoundingArea(new BoundingBox(block->GetBox().x, block->GetBox().y, block->GetBox().x + block->GetBox().w, block->GetBox().y + block->GetBox().h));
	block->GetGravityHandler().setGravityAddicted(false);
	block->SetFormStateId(BRICK);

	CollisionChecker::GetSingleton().Register(mario, block,
		[x, y, game](Sprite* s1, Sprite* s2) {
			if (s2->GetFormStateId() != EMPTY_BLOCK) {
				int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
				int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();
				int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
				int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
				int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
				int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

				if (s1_x1 > s2_x1 - 12 && s1_x2 < s2_x2 + 12 && s1_y1 + 3 >= s2_y2) {
					s2->Move(0, -4);
					s2->SetFormStateId(MOVED_BLOCK);
					jump->Stop();
					if (jump_anim != nullptr)
						jump->deleteCurrAnimation();
					al_start_timer(blockTimer);

					int giftNum = rand() % 100;
					if(giftNum < 10)
						create_starman(x - action_layer->GetViewWindow().x, y - 16);
					else if(giftNum < 20)
						create_1UP_mushroom(x - action_layer->GetViewWindow().x, y - 16, game);
					else if (giftNum < 40) {}
					else if(giftNum < 70)
						create_super_mushroom(x - action_layer->GetViewWindow().x, y - 16);
					else if(giftNum < 100)
						create_coin_sprite(x - action_layer->GetViewWindow().x, y - 16, game);

					if (!SpriteManager::GetSingleton().GetTypeList("powerup").empty()) {
						std::vector<std::string> block_types = { "brick" , "block" };
						for(std::string type : block_types)
							for (Sprite* brick : SpriteManager::GetSingleton().GetTypeList(type))
								for (Sprite* powerup : SpriteManager::GetSingleton().GetTypeList("powerup"))
									collisionBlockWithPowerUps(brick, powerup);
					}
				}
			}
		}
	);
}

void app::create_coin_sprite(int x, int y, Game* game) {
	Sprite* coin = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("blocks.coin"), "coin");
	SpriteManager::GetSingleton().Add(coin);

	coin->SetHasDirectMotion(true);
	coin->SetStateId(IDLE_STATE);
	coin->SetZorder(1);
	coin->SetBoundingArea(new BoundingCircle(coin->GetBox().x + (coin->GetBox().w / 2), coin->GetBox().y + (coin->GetBox().h / 2), 5));
	coin->GetGravityHandler().setGravityAddicted(false);
	coin->SetFormStateId(COIN);

	CollisionChecker::GetSingleton().Register(mario, coin,
		[game](Sprite* s1, Sprite* s2) {
			CollisionChecker::GetSingleton().Cancel(s1, s2);
			s2->SetFormStateId(DELETE);
			game->addCoin();
			game->addPoints(game_params.coin_points);
		}
	);
}

//crete powerups
void app::create_super_mushroom(int x, int y) {
	Sprite* powerup = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("powerups.super"), "powerup");
	SpriteManager::GetSingleton().Add(powerup);

	powerup->SetStateId(WALKING_STATE);
	powerup->SetZorder(0);
	powerup->SetBoundingArea(new BoundingBox(powerup->GetBox().x, powerup->GetBox().y, powerup->GetBox().x + powerup->GetBox().w, powerup->GetBox().y + powerup->GetBox().h));
	powerup->GetGravityHandler().setGravityAddicted(true);

	powerup->GetGravityHandler().SetOnSolidGround([powerup](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, powerup->GetStateId());
	});

	powerup->SetMover([powerup](const Rect& pos, int* dx, int* dy) {
		Rect posOnGrid{
			pos.x + action_layer->GetViewWindow().x,
			pos.y + action_layer->GetViewWindow().y,
			pos.w,
			pos.h,
		};
		action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
		if (*dx == 0) {
			powerup->lastMovedRight = !powerup->lastMovedRight;
		}
		powerup->SetPos(pos.x + *dx, pos.y + *dy);
	});

	CollisionChecker::GetSingleton().Register(mario, powerup,
		[](Sprite* s1, Sprite* s2) {
			CollisionChecker::GetSingleton().Cancel(s1, s2);
			s2->SetFormStateId(DELETE);
			if (s1->GetFormStateId() == SUPER_MARIO || s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER) {
				return; //if mario is already big, just collect the points
			}

			//s1 must be mario
			if (s1->GetFormStateId() == SMALL_MARIO)
				s1->SetFormStateId(SUPER_MARIO);
			else
				s1->SetFormStateId(INVINCIBLE_MARIO_SUPER);

			s1->Set_Str_StateId("Mario_big");
			s1->Move(0,-16);
			s1->SetBoxDimentions(16, 32);
			s1->ReplaceBoundingArea(new BoundingBox(s1->GetBox().x, s1->GetBox().y, s1->GetBox().x + s1->GetBox().w, s1->GetBox().y + s1->GetBox().h));
		}
	);
	powerup->GetGravityHandler().Check(powerup->GetBox()); //activte gravity
}

void app::create_1UP_mushroom(int x, int y, Game* game) {
	Sprite* powerup = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("powerups.1up"), "powerup");
	SpriteManager::GetSingleton().Add(powerup);

	powerup->SetStateId(WALKING_STATE);
	powerup->SetZorder(0);
	powerup->SetBoundingArea(new BoundingBox(powerup->GetBox().x, powerup->GetBox().y, powerup->GetBox().x + powerup->GetBox().w, powerup->GetBox().y + powerup->GetBox().h));
	powerup->GetGravityHandler().setGravityAddicted(true);

	powerup->GetGravityHandler().SetOnSolidGround([powerup](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, powerup->GetStateId());
		});

	powerup->SetMover(
		[powerup](const Rect& pos, int* dx, int* dy) {
			Rect posOnGrid{
				pos.x + action_layer->GetViewWindow().x,
				pos.y + action_layer->GetViewWindow().y,
				pos.w,
				pos.h,
			};
			action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
			if (*dx == 0) {
				powerup->lastMovedRight = !powerup->lastMovedRight;
			}
			powerup->SetPos(pos.x + *dx, pos.y + *dy);
		});

	CollisionChecker::GetSingleton().Register(mario, powerup,
		[game](Sprite* s1, Sprite* s2) {
			CollisionChecker::GetSingleton().Cancel(s1, s2);
			s2->SetFormStateId(DELETE);
			game->addLife();
		}
	);
	powerup->GetGravityHandler().Check(powerup->GetBox()); //activte gravity
}

FlashAnimator* star_animator = nullptr;
void app::create_starman(int x, int y) {
	Sprite* powerup = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("powerups.star"), "powerup");
	SpriteManager::GetSingleton().Add(powerup);

	powerup->SetStateId(WALKING_STATE);
	powerup->SetBoundingArea(new BoundingBox(powerup->GetBox().x, powerup->GetBox().y, powerup->GetBox().x + powerup->GetBox().w, powerup->GetBox().y + powerup->GetBox().h));
	powerup->GetGravityHandler().setGravityAddicted(true);

	powerup->GetGravityHandler().SetOnSolidGround([powerup](const Rect& r) {
		Rect posOnGrid{
			r.x + action_layer->GetViewWindow().x,
			r.y + action_layer->GetViewWindow().y,
			r.w,
			r.h,
		};

		return action_layer->GetGrid()->IsOnSolidGround(posOnGrid, powerup->GetStateId());
		});

	powerup->SetMover(
		[powerup](const Rect& pos, int* dx, int* dy) {
			Rect posOnGrid{
				pos.x + action_layer->GetViewWindow().x,
				pos.y + action_layer->GetViewWindow().y,
				pos.w,
				pos.h,
			};
			action_layer->GetGrid()->FilterGridMotion(posOnGrid, dx, dy);
			if (*dx == 0) {
				powerup->lastMovedRight = !powerup->lastMovedRight;
			}
			powerup->SetPos(pos.x + *dx, pos.y + *dy);
		});

	//invincible mario 
	CollisionChecker::GetSingleton().Register(mario, powerup,
		[](Sprite* s1, Sprite* s2) {
			CollisionChecker::GetSingleton().Cancel(s1, s2);
			s2->SetFormStateId(DELETE); //delete the star

			if (s1->GetFormStateId() == SMALL_MARIO)
				s1->SetFormStateId(INVINCIBLE_MARIO_SMALL);
			else if (s1->GetFormStateId() == SUPER_MARIO)
				s1->SetFormStateId(INVINCIBLE_MARIO_SUPER);
			else { //TODO: mario is already invinsible. Do something
				if (star_animator == nullptr) return; //no mans land
				star_animator->ResetCurrRep(); //reset
				return;
			}

			FlashAnimator* animator = new FlashAnimator();
			AnimatorManager::GetSingleton().Register(animator);
			star_animator = animator;


			animator->SetOnAction([s1](Animator* animator, const Animation& anim) {
				s1->SetVisibility(!s1->IsVisible());
			});

			animator->SetOnFinish([s1](Animator* animator) {
				s1->SetVisibility(true);
				star_animator = nullptr;
				((FlashAnimator*)animator)->deleteCurrAnimation();
				AnimatorManager::GetSingleton().Cancel(animator);
				animator->Destroy();
				if (s1->GetFormStateId() == INVINCIBLE_MARIO_SMALL)
					s1->SetFormStateId(SMALL_MARIO);
				else if (s1->GetFormStateId() == INVINCIBLE_MARIO_SUPER)
					s1->SetFormStateId(SUPER_MARIO);
			});
			animator->Start(new FlashAnimation("flash_star", game_params.invinsible_mario_duration * 10, 100, 100), GetGameTime());
		}
	);
	powerup->GetGravityHandler().Check(powerup->GetBox()); //activte gravity
}

//---------------------------------------------------------------------------
//-------------------From Here Is The Pipe Creation--------------------------
//---------------------------------------------------------------------------

void app::MoveScene(int new_screen_x, int new_screen_y, int new_mario_x, int new_mario_y) {
	Sprite* mario = SpriteManager::GetSingleton().GetTypeList("mario").front();
	mario->SetHasDirectMotion(true);
	mario->Move(new_mario_x - mario->GetBox().x, new_mario_y - mario->GetBox().y);
	mario->SetHasDirectMotion(false);

	auto sprites = SpriteManager::GetSingleton().GetTypeList("pipe");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("goomba");

	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->SetHasDirectMotion(true);
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
		sprite->SetHasDirectMotion(false);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("green_koopa_troopa");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->SetHasDirectMotion(true);
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
		sprite->SetHasDirectMotion(false);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("red_koopa_troopa");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->SetHasDirectMotion(true);
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
		sprite->SetHasDirectMotion(false);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("piranha_plant");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0); //they anyway have direct motion
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("brick");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("block");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("powerup");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->SetHasDirectMotion(true);
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
		sprite->SetHasDirectMotion(false);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("coin");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pipes)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
	}

	sprites = SpriteManager::GetSingleton().GetTypeList("pole");
	for (auto sprite : sprites) { // move the sprites the opposite directions (f.e. pole)
		sprite->Move(-(new_screen_x - action_layer->GetViewWindow().x), 0);
	}

	circular_background->Scroll(new_screen_x - action_layer->GetViewWindow().x);
	underground_layer->SetViewWindow(Rect{ new_screen_x, new_screen_y, underground_layer->GetViewWindow().w, underground_layer->GetViewWindow().h });
	action_layer->SetViewWindow(Rect{ new_screen_x, new_screen_y, action_layer->GetViewWindow().w, action_layer->GetViewWindow().h });
}

Sprite* app::LoadPipeCollision(Sprite* mario, string pipes) {
	vector<string> coordinates = splitString(pipes.substr(1, pipes.length()), " ");
	int x = atoi(coordinates[0].c_str());
	int y = atoi(coordinates[1].c_str());
	int new_screen_x = atoi(coordinates[2].c_str());
	int new_screen_y = atoi(coordinates[3].c_str());
	int new_mario_x = atoi(coordinates[4].c_str());
	int new_mario_y = atoi(coordinates[5].c_str());


	Sprite* tmp = nullptr;
	switch (pipes.at(0)) {
	case 'u':
		tmp = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("Pipe.up"), "pipe");
		CollisionChecker::GetSingleton().Register(mario, tmp, [mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Sprite* s1, Sprite* s2) {

			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if ((s2_y2 >= s1_y1) && (s1_x1 >= s2_x1) && (s1_x2 <= s2_x2) && keys[ALLEGRO_KEY_S]) { //if mario on top of the pipe
				//PLAY ANIMATION HERE

				if (pipe_movement->GetAnim() != nullptr)
					return;
				pipe_movement->Start(new MovingAnimation("pipe.up", 32, 0, 1, 30), GetGameTime());

				pipe_movement->SetOnFinish([mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Animator* animator) {
					pipe_movement->deleteCurrAnimation();
					mario->SetHasDirectMotion(false);
					mario->SetFrame(0);
					mario->GetGravityHandler().setGravityAddicted(true);
					MoveScene(new_screen_x, new_screen_y, new_mario_x, new_mario_y);
					AnimatorManager::GetSingleton().MarkAsRunning(walk);
					disable_input = false;
					});

			}
			});
		break;
	case 'd':
		tmp = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("Pipe.down"), "pipe");
		CollisionChecker::GetSingleton().Register(mario, tmp, [mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Sprite* s1, Sprite* s2) {

			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();
			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s1_x2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX2();
			int s2_x1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();

			if ((s2_y2 <= s1_y1) && (s1_x1 >= s2_x1) && (s1_x2 <= s2_x2) && (keys[ALLEGRO_KEY_W] || keys[ALLEGRO_KEY_UP])) { //if mario bellow the pipe
				if (pipe_movement->GetAnim() != nullptr)
					return;
				pipe_movement->Start(new MovingAnimation("pipe.down", 32, 0, -1, 30), GetGameTime());

				pipe_movement->SetOnFinish([mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Animator* animator) {
					pipe_movement->deleteCurrAnimation();
					mario->SetHasDirectMotion(false);
					mario->SetFrame(0);
					mario->GetGravityHandler().setGravityAddicted(true);
					MoveScene(new_screen_x, new_screen_y, new_mario_x, new_mario_y);
					AnimatorManager::GetSingleton().MarkAsRunning(walk);
					disable_input = false;
					});
			}
			});
		break;
	case 'l':
		tmp = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("Pipe.left"), "pipe");
		CollisionChecker::GetSingleton().Register(mario, tmp, [mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Sprite* s1, Sprite* s2) {

			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();
			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();

			if ((s2_x2 >= s1_x1) && (s1_y1 >= s2_y1) && (s1_y2 <= s2_y2) && (keys[ALLEGRO_KEY_D] || keys[ALLEGRO_KEY_RIGHT])) { //if mario on left of the pipe
				if (pipe_movement->GetAnim() != nullptr)
					return;
				pipe_movement->Start(new MovingAnimation("pipe.left", 32, 1, 0, 30), GetGameTime());

				pipe_movement->SetOnFinish([mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Animator* animator) {
					pipe_movement->deleteCurrAnimation();
					mario->SetHasDirectMotion(false);
					mario->SetFrame(0);
					mario->GetGravityHandler().setGravityAddicted(true);
					MoveScene(new_screen_x, new_screen_y, new_mario_x, new_mario_y);
					AnimatorManager::GetSingleton().MarkAsRunning(walk);
					disable_input = false;
					});
			}
			});
		break;
	case 'r':
		tmp = new Sprite(x, y, AnimationFilmHolder::GetInstance().GetFilm("Pipe.right"), "pipe");
		CollisionChecker::GetSingleton().Register(mario, tmp, [mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Sprite* s1, Sprite* s2) {

			int s1_x1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getX1();
			int s2_x2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getX2();
			int s1_y1 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY1();
			int s1_y2 = ((const BoundingBox*)(s1->GetBoundingArea()))->getY2();
			int s2_y1 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY1();
			int s2_y2 = ((const BoundingBox*)(s2->GetBoundingArea()))->getY2();

			if ((s2_x2 <= s1_x1) && (s1_y1 >= s2_y1) && (s1_y2 <= s2_y2) && (keys[ALLEGRO_KEY_A] || keys[ALLEGRO_KEY_LEFT])) { //if mario on right of the pipe
				if (pipe_movement->GetAnim() != nullptr)
					return;
				pipe_movement->Start(new MovingAnimation("pipe.right", 32, -1, 0, 30), GetGameTime());

				pipe_movement->SetOnFinish([mario, new_screen_x, new_screen_y, new_mario_x, new_mario_y](Animator* animator) {
					pipe_movement->deleteCurrAnimation();
					mario->SetHasDirectMotion(false);
					mario->SetFrame(0);
					mario->GetGravityHandler().setGravityAddicted(true);
					MoveScene(new_screen_x, new_screen_y, new_mario_x, new_mario_y);
					AnimatorManager::GetSingleton().MarkAsRunning(walk);
					disable_input = false;
					});
			}
			});
		break;
	}

	assert(tmp);
	tmp->SetHasDirectMotion(true);
	tmp->GetGravityHandler().setGravityAddicted(false);
	tmp->SetZorder(1);
	tmp->SetBoundingArea(new BoundingBox(tmp->GetBox().x, tmp->GetBox().y, tmp->GetBox().x + tmp->GetBox().w, tmp->GetBox().y + tmp->GetBox().h));

	return tmp;
}