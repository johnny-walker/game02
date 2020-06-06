/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "game_level.h"
#include "ball_object.h"
#include "sprite_renderer.h"
#include "resource_manager.h"
#include "particle_generator.h"
#include "post_processor.h"

// Represents the current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Represents the four possible (collision) directions
enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Defines a Collision typedef that represents collision data
// <collision?, what direction?, difference vector center - closest point>
typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;


// Initial size/velocity of the player paddle
const glm::vec2 PLAYER_SIZE(100, 20);
const GLfloat PLAYER_VELOCITY(500.0f);

// 初始化球的速度, 半径
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
const GLfloat BALL_RADIUS = 12.5f;

// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
    SpriteRenderer      *Renderer;
    ParticleGenerator   *Particles;

    GameObject          *Player;
    BallObject          *Ball;
    
    PostProcessor       *Effects;
    GLfloat             ShakeTime = 0.0f;

public:
    // Game state
    GameState              State;
    GLboolean              Keys[1024];
    GLuint                 Width, Height;
    std::vector<GameLevel> Levels;
    GLuint                 Level;
    
    // constructor/destructor
    Game(unsigned int width, unsigned int height);
    ~Game();
    // initialize game state (load all shaders/textures/levels)
    void Init();
    
    // game loop
    void ProcessInput(float dt);
    void Update(float dt);
    void Render();
    
    // Reset
    void ResetLevel();
    void ResetPlayer();
    
private:
    // Collision detection
    void DoCollisions();
    GLboolean CheckCollision(GameObject &one, GameObject &two);
    Collision CheckCollision(BallObject &one, GameObject &two);
    Direction VectorDirection(glm::vec2 closest);
};

#endif
