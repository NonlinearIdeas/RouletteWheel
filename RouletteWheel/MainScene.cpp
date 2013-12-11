/********************************************************************
 * File   : MainScene.cpp
 * Project: Multiple
 *
 ********************************************************************
 * Created on 9/21/13 By Nonlinear Ideas Inc.
 * Copyright (c) 2013 Nonlinear Ideas Inc. All rights reserved.
 ********************************************************************
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and
 *    must not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 */

#include "MainScene.h"
#include "Box2DDebugDrawLayer.h"
#include "GridLayer.h"
#include "DebugLinesLayer.h"
#include "Notifier.h"
#include "Viewport.h"
#include "MathUtilities.h"


MainScene::MainScene()
{
}

MainScene::~MainScene()
{
}


void MainScene::CreatePhysics()
{
   // Set up the viewport
   static const float32 worldSizeMeters = 10.0;
   
   // Initialize the Viewport
   Viewport::Instance().Init(worldSizeMeters);
   Viewport::Instance().SetScale(1.0);
   
   Vec2 gravity(0.0,0.0);
   _world = new b2World(gravity);
   // Do we want to let bodies sleep?
   // No for now...makes the debug layer blink
   // which is annoying.
   _world->SetAllowSleeping(false);
   _world->SetContinuousPhysics(true);
}

bool MainScene::init()
{
   
   // Create physical world
   CreatePhysics();
   
   // Create the body with fixtures
   CreateBody();
   
   // Adding the debug lines so that we can draw the path followed.
   addChild(DebugLinesLayer::create());
   
   // Box2d Debug
   addChild(Box2DDebugDrawLayer::create(_world));
   
   // Grid
   addChild(GridLayer::create());
   
   return true;
}

Vec2 CalculateAverage(vector<Vec2>& points)
{
   Vec2 sum(0,0);
   for(int idx = 0; idx < points.size(); idx++)
   {
      sum += points[idx];
   }
   if(points.size() > 0)
      sum *= 1.0/(points.size());
   return sum;
}

void MainScene::CreateBody()
{
   const float32 INNER_RADIUS = 2.50;
   const float32 OUTER_RADIUS = 3.0;
   const float32 BALL_RADIUS = 0.1;
   const uint32 DIVISIONS = 36;
   
   Vec2 position(0,0);
   
   // Create the body.
   b2BodyDef bodyDef;
   bodyDef.position = position;
   bodyDef.type = b2_dynamicBody;
   _body = _world->CreateBody(&bodyDef);
   assert(_body != NULL);
   
   // Now attach fixtures to the body.
   FixtureDef fixtureDef;
   fixtureDef.density = 1.0;
   fixtureDef.friction = 1.0;
   fixtureDef.restitution = 0.9;
   fixtureDef.isSensor = false;
   
   // Inner circle.
   b2CircleShape circleShape;
   circleShape.m_radius = INNER_RADIUS;
   fixtureDef.shape = &circleShape;
   _body->CreateFixture(&fixtureDef);
   
   // Outer shape.
   b2ChainShape chainShape;
   vector<Vec2> vertices;
   const float32 SPIKE_DEGREE = 2*M_PI/180;
   for(int idx = 0; idx < DIVISIONS; idx++)
   {
      float32 angle = ((M_PI*2)/DIVISIONS)*idx;
      float32 xPos, yPos;
      
      xPos = OUTER_RADIUS*cosf(angle-SPIKE_DEGREE);
      yPos = OUTER_RADIUS*sinf(angle-SPIKE_DEGREE);
      vertices.push_back(Vec2(xPos,yPos));

      xPos = OUTER_RADIUS*cosf(angle)*.98;
      yPos = OUTER_RADIUS*sinf(angle)*.98;
      vertices.push_back(Vec2(xPos,yPos));
      
      
      xPos = OUTER_RADIUS*cosf(angle+SPIKE_DEGREE);
      yPos = OUTER_RADIUS*sinf(angle+SPIKE_DEGREE);
      vertices.push_back(Vec2(xPos,yPos));
   }
   vertices.push_back(vertices[0]);
   chainShape.CreateChain(&vertices[0], vertices.size());
   fixtureDef.shape = &chainShape;
   _body->CreateFixture(&fixtureDef);
   
   // Create some "spikes" for the ball to bounce off of.
   
   // Start it spinning
   _body->SetAngularVelocity(M_PI/8);
   
   // NOW create a ball to bounce around inside...
   bodyDef.position = Vec2((INNER_RADIUS+OUTER_RADIUS)/2,0);
   _ballBody = _world->CreateBody(&bodyDef);
   circleShape.m_radius = BALL_RADIUS;
   fixtureDef.shape = &circleShape;
   _ballBody->CreateFixture(&fixtureDef);
   
   // Give it some velocity so it starts to bounce.
   _ballBody->SetLinearVelocity(Vec2(-0.5,0.5));
}

void MainScene::UpdateBody()
{
   _ballBody->SetAngularVelocity(M_PI);
}

MainScene* MainScene::create()
{
   MainScene *pRet = new MainScene();
   if (pRet && pRet->init())
   {
      pRet->autorelease();
      return pRet;
   }
   else
   {
      CC_SAFE_DELETE(pRet);
      return NULL;
   }
}

void MainScene::onEnter()
{
   CCScene::onEnter();
}

void MainScene::onExit()
{
   CCScene::onExit();
}

void MainScene::onEnterTransitionDidFinish()
{
   CCScene::onEnterTransitionDidFinish();
   // Schedule Updates
   scheduleUpdate();
}

void MainScene::onExitTransitionDidStart()
{
   CCScene::onExitTransitionDidStart();
   
   // Turn off updates
   unscheduleUpdate();
}

static void AdjustNodeScale(CCNode* node, float32 entitySizeMeters, float32 ptmRatio)
{
   CCSize nodeSize = node->getContentSize();
   float32 maxSizePixels = max(nodeSize.width,nodeSize.height);
   assert(maxSizePixels >= 1.0);
   float32 scale = (entitySizeMeters*ptmRatio/maxSizePixels);
   
   node->setScale(scale);
   /*
    CCLOG("Adjusting Node Scale: em:%f, msp:%f, ptm:%f, scale:%f",
    entitySizeMeters,
    maxSizePixels,
    ptmRatio,
    scale
    );
    */
}


void MainScene::UpdatePhysics()
{
   const int velocityIterations = 8;
   const int positionIterations = 1;
   float32 fixedDT = SECONDS_PER_TICK;
   // Instruct the world to perform a single step of simulation. It is
   // generally best to keep the time step and iterations fixed.
   _world->Step(fixedDT, velocityIterations, positionIterations);
}

void MainScene::update(float dt)
{
   UpdatePhysics();
   UpdateBody();
}

