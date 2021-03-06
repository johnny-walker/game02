/*******************************************************************
** This code is part of Breakout.
**
** Breakout is free software: you can redistribute it and/or modify
** it under the terms of the CC BY 4.0 license as published by
** Creative Commons, either version 4 of the License, or (at your
** option) any later version.
******************************************************************/
#include "game.h"

//#include <string>
using namespace std;

string path         = string("/users/albert/openGL/game02/game02");
//shaders
string vertex01     = path + "/shaders/sprite.vs";
string fragment01   = path + "/shaders/sprite.fs";
string vertex02     = path + "/shaders/particle.vs";
string fragment02   = path + "/shaders/particle.fs";
string vertex03     = path + "/shaders/post_processing.vs";
string fragment03   = path + "/shaders/post_processing.fs";

//texture images
string background   = path + "/textures/background.jpg";
string block        = path + "/textures/block.png";
string block_solid  = path + "/textures/block_solid.png";
string paddle       = path + "/textures/paddle.png";
string ball         = path + "/textures/spirit_bomb.png";
string particle     = path + "/textures/particle.png";

//levels
#define TOTAL_LEVELS   4
string level1       = path + "/levels/one.lvl";
string level2       = path + "/levels/two.lvl";
string level3       = path + "/levels/three.lvl";
string level4       = path + "/levels/four.lvl";
string testFive     = path + "/levels/five.lvl";


Game::Game(unsigned int width, unsigned int height)
    : State(GAME_ACTIVE), Keys(), Width(width), Height(height)
{ 
}

Game::~Game()
{
    delete Renderer;
    delete Player;
    delete Ball;
    delete Particles;
    delete Effects;
}

void Game::Init()
{
    // Load shaders
    ResourceManager::LoadShader(vertex01.c_str(), fragment01.c_str(), nullptr, "sprite");
    ResourceManager::LoadShader(vertex02.c_str(), fragment02.c_str(), nullptr, "particle");
    ResourceManager::LoadShader(vertex03.c_str(), fragment03.c_str(), nullptr, "postprocessing");
    
    // Load textures
    ResourceManager::LoadTexture(background.c_str(),  GL_FALSE, "background");
    ResourceManager::LoadTexture(block.c_str(),       GL_FALSE, "block");
    ResourceManager::LoadTexture(block_solid.c_str(), GL_FALSE, "block_solid");
    ResourceManager::LoadTexture(paddle.c_str(),      GL_TRUE,  "paddle");
    ResourceManager::LoadTexture(ball.c_str(),        GL_TRUE,  "ball");
    ResourceManager::LoadTexture(particle.c_str(),    GL_TRUE,  "particle");

    // Configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("image", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    
    // Set render-specific controls
    Renderer  = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
    Effects   = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);

    // Load levels
    GameLevel one;
    GameLevel two;
    GameLevel three;
    GameLevel four;
    GameLevel testLevel;
    
    one.Load(level1.c_str(), this->Width, this->Height * 0.5);
    two.Load(level2.c_str(), this->Width, this->Height * 0.5);
    three.Load(level3.c_str(), this->Width, this->Height * 0.5);
    four.Load(level4.c_str(), this->Width, this->Height * 0.5);
    testLevel.Load(testFive.c_str(), this->Width, this->Height * 0.5);
    
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Levels.push_back(testLevel);
    this->Level = 2;
    
    // Configure game objects
    glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, ResourceManager::GetTexture("ball"));
}

void Game::Update(GLfloat dt)
{
    Ball->Move(dt, this->Width);
    
    // Check for collisions
    this->DoCollisions();
    
    // Update particles
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
    
    // Reduce shake time
    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = GL_FALSE;
    }
    // Check loss condition
    if (Ball->Position.y >= this->Height) // Did ball reach bottom edge?
    {
        this->ResetLevel();
        this->ResetPlayer();
    }
    
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        this->ResetLevel();
        this->ResetPlayer();
        //Effects->Chaos = GL_TRUE;
        this->State = GAME_MENU;
        this->Level = (this->Level + 1) % TOTAL_LEVELS;
    }
}

// press W/S to choose level
// press Enter to activate the game
// press Space to release ball
void Game::ProcessInput(GLfloat dt)
{
    if (this->State == GAME_MENU || this->State == GAME_WIN)
    {
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
        }
        if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
        {
            this->Level = (this->Level + 1) % TOTAL_LEVELS;
            this->KeysProcessed[GLFW_KEY_W] = GL_TRUE;
        }
        if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
        {
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = 3;
            this->KeysProcessed[GLFW_KEY_S] = GL_TRUE;
        }
    }
    
    if (this->State == GAME_ACTIVE)
    {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // 移动玩家挡板
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0)
            {
                Player->Position.x -= velocity;
                if (Ball->Stuck)
                    Ball->Position.x -= velocity;
            }
        }
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
            {
                Player->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;
            }
        }
        if (this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }
}

void Game::Render()
{
    if (this->State == GAME_MENU || this->State == GAME_ACTIVE)
    {
        // Begin rendering to postprocessing quad
        Effects->BeginRender();
        
        // Draw background
        Renderer->DrawSprite(ResourceManager::GetTexture("background"), glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);

        // Draw level
        this->Levels[this->Level].Draw(*Renderer);

        // Draw player
        Player->Draw(*Renderer);

        // Draw particles
        Particles->Draw();

        // Draw ball
        Ball->Draw(*Renderer);
        
        // End rendering to postprocessing quad
        Effects->EndRender();
        
        // Render postprocessing quad
        Effects->Render(glfwGetTime());
    }
}

void Game::ResetLevel()
{   /*
    if (this->Level == 0)
        this->Levels[0].Load(level1.c_str(), this->Width, this->Height * 0.5f);
    else if (this->Level == 1)
        this->Levels[1].Load(level2.c_str(), this->Width, this->Height * 0.5f);
    else if (this->Level == 2)
        this->Levels[2].Load(level3.c_str(), this->Width, this->Height * 0.5f);
    else if (this->Level == 3)
        this->Levels[3].Load(level4.c_str(), this->Width, this->Height * 0.5f);
    */
    this->Levels[this->Level].Load(this->Levels[this->Level].LevelName->c_str(), this->Width, this->Height * 0.5f);
}

void Game::ResetPlayer()
{
    // Reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)), INITIAL_BALL_VELOCITY);
}


void Game::DoCollisions()
{
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*Ball, box);
            if (std::get<0>(collision)) // If collision is true
            {
                // Destroy block if not solid
                if (!box.IsSolid)
                {
                    box.Destroyed = GL_TRUE;
                }
                else
                {   // if block is solid, enable shake effect
                    ShakeTime = 0.05f;
                    Effects->Shake = GL_TRUE;
                }
                // Collision resolution
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (dir == LEFT || dir == RIGHT) // Horizontal collision
                {
                    Ball->Velocity.x = -Ball->Velocity.x; // Reverse horizontal velocity
                    // Relocate
                    GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
                    if (dir == LEFT)
                        Ball->Position.x += penetration; // Move ball to right
                    else
                        Ball->Position.x -= penetration; // Move ball to left;
                }
                else // Vertical collision
                {
                    Ball->Velocity.y = -Ball->Velocity.y; // Reverse vertical velocity
                    // Relocate
                    GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
                    if (dir == UP)
                        Ball->Position.y -= penetration; // Move ball bback up
                    else
                        Ball->Position.y += penetration; // Move ball back down
                }
            }
        }
    }
    // Also check collisions for player pad (unless stuck)
    Collision result = CheckCollision(*Ball, *Player);
    if (!Ball->Stuck && std::get<0>(result))
    {
        // Check where it hit the board, and change velocity based on where it hit the board
        GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
        GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        GLfloat percentage = distance / (Player->Size.x / 2);
        // Then move accordingly
        GLfloat strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        //Ball->Velocity.y = -Ball->Velocity.y;
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // Keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
        // Fix sticky paddle
        Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
    }
}

GLboolean Game::CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
    // Collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
    two.Position.x + two.Size.x >= one.Position.x;
    // Collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
    two.Position.y + two.Size.y >= one.Position.y;
    // Collision only if on both axes
    return collisionX && collisionY;
}

Collision Game::CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
    // Get center point circle first
    glm::vec2 center(one.Position + one.Radius);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    // Get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // Now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // Now retrieve vector between center circle and closest point AABB and check if length < radius
    difference = closest - center;
    
    if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    else
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

// Calculates which direction a vector is facing (N,E,S or W)
Direction Game::VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),    // up
        glm::vec2(1.0f, 0.0f),    // right
        glm::vec2(0.0f, -1.0f),    // down
        glm::vec2(-1.0f, 0.0f)    // left
    };
    GLfloat max = 0.0f;
    GLuint best_match = -1;
    for (GLuint i = 0; i < 4; i++)
    {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}
