#include "Background.h"
#include <cmath>
#include <cstring>

static const float kBGPI = 3.14159265f;

void Background::generateStars(int screenW, int screenH) {
    stars.clear();
    int count = 200;
    for (int i = 0; i < count; ++i) {
        ParallaxStar s;
        s.pos        = {(float)GetRandomValue(0, screenW), (float)GetRandomValue(0, screenH*3/4)};
        s.brightness = GetRandomValue(30, 100) / 100.0f;
        s.layer      = GetRandomValue(0, 100) / 100.0f;
        s.twinkle    = GetRandomValue(0, 628) / 100.0f;
        stars.push_back(s);
    }
    // Data stream drops
    dataDrops.clear();
    for (int i = 0; i < 60; ++i) {
        DataStreamDrop d;
        d.x     = (float)GetRandomValue(0, screenW);
        d.y     = (float)GetRandomValue(-screenH, screenH);
        d.speed = GetRandomValue(80, 220) * 1.0f;
        d.len   = GetRandomValue(4, 14);
        d.alpha = GetRandomValue(30, 80) / 100.0f;
        dataDrops.push_back(d);
    }
}

void Background::generate(ZoneID zone, int mapW, int mapH, int tileSize) {
    envObjects.clear();
    ashParticles.clear();
    ashVelocities.clear();
    lastZone = zone;

    generateStars(1280, 720);
    placeObjects(zone, mapW, mapH, tileSize);

    // Ash / ember particles
    for (int i = 0; i < 120; ++i) {
        ashParticles.push_back({
            (float)GetRandomValue(0, mapW * tileSize),
            (float)GetRandomValue(0, mapH * tileSize)
        });
        ashVelocities.push_back({
            (float)GetRandomValue(-20, 20),
            (float)GetRandomValue(-40, -10)
        });
    }
}

void Background::placeObjects(ZoneID zone, int mapW, int mapH, int tileSize) {
    int half = tileSize / 2;

    // Wrecked cars scattered around
    int numCars = (zone == ZoneID::LARuins) ? 18 :
                  (zone == ZoneID::KronosForge) ? 10 : 6;
    for (int i = 0; i < numCars; ++i) {
        EnvObject obj;
        obj.type     = EnvObject::Type::Car;
        obj.pos      = {(float)GetRandomValue(2*tileSize, (mapW-2)*tileSize),
                         (float)GetRandomValue(2*tileSize, (mapH-2)*tileSize)};
        obj.rotation = (float)GetRandomValue(0, 180);
        obj.scale    = GetRandomValue(80, 120) / 100.0f;
        // Zone-specific tints
        switch (zone) {
            case ZoneID::LARuins:       obj.tint = {80,70,60,255};  break;
            case ZoneID::KronosForge: obj.tint = {60,40,30,255};  break;
            default:                    obj.tint = {70,75,80,255};  break;
        }
        envObjects.push_back(obj);
    }

    // Rubble piles
    int numRubble = 25;
    for (int i = 0; i < numRubble; ++i) {
        EnvObject obj;
        obj.type  = EnvObject::Type::Rubble;
        obj.pos   = {(float)GetRandomValue(tileSize, (mapW-1)*tileSize),
                      (float)GetRandomValue(tileSize, (mapH-1)*tileSize)};
        obj.scale = GetRandomValue(50, 150) / 100.0f;
        switch (zone) {
            case ZoneID::LARuins:       obj.tint = {90,80,70,255};  break;
            case ZoneID::Bunker:        obj.tint = {50,70,50,255};  break;
            case ZoneID::KronosForge: obj.tint = {100,60,30,255}; break;
            case ZoneID::KronosNexus:  obj.tint = {60,40,100,255}; break;
        }
        envObjects.push_back(obj);
    }

    // Fire pits / burning wreckage
    int numFires = (zone == ZoneID::LARuins || zone == ZoneID::KronosForge) ? 12 : 4;
    for (int i = 0; i < numFires; ++i) {
        EnvObject obj;
        obj.type     = EnvObject::Type::FirePit;
        obj.pos      = {(float)GetRandomValue(2*tileSize, (mapW-2)*tileSize),
                         (float)GetRandomValue(2*tileSize, (mapH-2)*tileSize)};
        obj.hasFire  = true;
        obj.animTimer = (float)GetRandomValue(0, 100) / 10.0f;
        envObjects.push_back(obj);
    }

    // Lamp posts
    int numLamps = 14;
    for (int i = 0; i < numLamps; ++i) {
        EnvObject obj;
        obj.type     = EnvObject::Type::Lamppost;
        obj.pos      = {(float)GetRandomValue(2*tileSize, (mapW-2)*tileSize),
                         (float)GetRandomValue(2*tileSize, (mapH-2)*tileSize)};
        obj.hasFire  = (GetRandomValue(0,100) < 40); // 40% still lit
        envObjects.push_back(obj);
    }

    // Crates / supply boxes
    for (int i = 0; i < 16; ++i) {
        EnvObject obj;
        obj.type  = EnvObject::Type::Crate;
        obj.pos   = {(float)GetRandomValue(tileSize, (mapW-1)*tileSize),
                      (float)GetRandomValue(tileSize, (mapH-1)*tileSize)};
        switch (zone) {
            case ZoneID::Bunker:        obj.tint = {60,100,60,255}; break;
            case ZoneID::KronosForge: obj.tint = {120,50,20,255}; break;
            default:                    obj.tint = {80,80,80,255};  break;
        }
        envObjects.push_back(obj);
    }

    // KRONOS terminals (factory / core)
    if (zone == ZoneID::KronosForge || zone == ZoneID::KronosNexus) {
        for (int i = 0; i < 10; ++i) {
            EnvObject obj;
            obj.type  = EnvObject::Type::Terminal;
            obj.pos   = {(float)GetRandomValue(2*tileSize, (mapW-2)*tileSize),
                          (float)GetRandomValue(2*tileSize, (mapH-2)*tileSize)};
            envObjects.push_back(obj);
        }
    }
}

void Background::update(float dt, Camera2D cam) {
    time += dt;
    // Update data stream drops
    for (auto& d : dataDrops) {
        d.y += d.speed * dt;
        if (d.y > 740.0f) {
            d.y     = (float)GetRandomValue(-60, -10);
            d.x     = (float)GetRandomValue(0, 1280);
            d.speed = GetRandomValue(80, 220) * 1.0f;
        }
    }
    (void)cam; // suppress unused warning from original signature
    int N = (int)ashParticles.size();
    for (int i = 0; i < N; ++i) {
        ashParticles[i].x += ashVelocities[i].x * dt;
        ashParticles[i].y += ashVelocities[i].y * dt;
        // drift direction slowly
        ashVelocities[i].x += (float)GetRandomValue(-5, 5) * dt;
        // wrap
        if (ashParticles[i].y < -10)  ashParticles[i].y = 2600;
        if (ashParticles[i].x < -10)  ashParticles[i].x = 2590;
        if (ashParticles[i].x > 2610) ashParticles[i].x = 0;
    }
    // update fire anim timers
    for (auto& obj : envObjects)
        obj.animTimer += dt;
}

// â”€â”€â”€ Sky â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Background::drawSky(int screenW, int screenH, ZoneID zone) const {
    // Default seguro (evita cores nao-inicializadas para zonas fora do switch)
    Color topA = {6,6,12,255},  topB = {16,16,28,255};
    Color midA = {26,26,42,255}, midB = {40,40,60,255};
    switch (zone) {
        case ZoneID::LARuins:
            topA = {10, 5, 5, 255};  topB = {35, 12, 8, 255};
            midA = {55, 20, 10, 255}; midB = {80, 35, 15, 255};
            break;
        case ZoneID::Bunker:
            topA = {5, 12, 5, 255};  topB = {12, 28, 12, 255};
            midA = {20, 45, 20, 255}; midB = {30, 60, 30, 255};
            break;
        case ZoneID::KronosForge:
            topA = {15, 6, 2, 255};  topB = {40, 16, 5, 255};
            midA = {65, 28, 8, 255};  midB = {90, 40, 12, 255};
            break;
        case ZoneID::KronosNexus:
            topA = {5, 3, 12, 255};  topB = {15, 8, 35, 255};
            midA = {30, 12, 70, 255}; midB = {50, 20, 100, 255};
            break;
        case ZoneID::Cemetery:
            topA = {6, 8, 14, 255};  topB = {14, 18, 28, 255};
            midA = {22, 28, 38, 255}; midB = {34, 40, 52, 255};
            break;
        case ZoneID::CursedFarm:
            topA = {12, 10, 6, 255}; topB = {28, 22, 12, 255};
            midA = {44, 36, 20, 255}; midB = {60, 50, 28, 255};
            break;
        case ZoneID::GhostCity:
            topA = {6, 8, 12, 255};  topB = {16, 20, 30, 255};
            midA = {26, 32, 44, 255}; midB = {38, 46, 60, 255};
            break;
        case ZoneID::DarkForest:
            topA = {4, 8, 6, 255};   topB = {10, 20, 12, 255};
            midA = {16, 30, 18, 255}; midB = {24, 42, 26, 255};
            break;
        case ZoneID::Catacombs:
            topA = {6, 5, 8, 255};   topB = {14, 10, 18, 255};
            midA = {24, 18, 28, 255}; midB = {34, 26, 40, 255};
            break;
        case ZoneID::AbandonedManor:
            topA = {8, 5, 10, 255};  topB = {18, 10, 22, 255};
            midA = {30, 18, 36, 255}; midB = {44, 26, 50, 255};
            break;
        case ZoneID::InfernoZone:
            topA = {18, 4, 2, 255};  topB = {48, 12, 4, 255};
            midA = {90, 28, 6, 255};  midB = {130, 45, 10, 255};
            break;
        default: break;
    }

    int h2 = screenH * 2 / 3;
    DrawRectangleGradientV(0, 0, screenW, h2, topA, topB);
    DrawRectangleGradientV(0, h2, screenW, screenH - h2, midA, midB);

    // Aurora/brilho atmosferico nas zonas escuras (camada sutil acima do horizonte)
    bool darkZone = (zone == ZoneID::Cemetery || zone == ZoneID::GhostCity ||
                     zone == ZoneID::DarkForest || zone == ZoneID::Catacombs ||
                     zone == ZoneID::AbandonedManor);
    if (darkZone) {
        for (int i = 0; i < 3; ++i) {
            float ax = fmodf(time * (6.0f + i*4.0f) + i*400.0f, (float)screenW + 300) - 150;
            float ay = screenH * 0.30f + i * 26.0f;
            float aw = 260.0f + i * 60.0f;
            float pulse = 0.04f + 0.03f * std::sin(time * 0.6f + i);
            Color au = (zone == ZoneID::DarkForest) ? Color{40,120,80,255}
                     : (zone == ZoneID::AbandonedManor) ? Color{120,40,120,255}
                                                        : Color{60,90,140,255};
            DrawEllipse((int)ax, (int)ay, (int)aw, 40, ColorAlpha(au, pulse));
        }
    }

    // Smoke/cloud wisps drifting
    for (int i = 0; i < 8; ++i) {
        float x = fmodf((float)(i * 220) + time * 12.0f, (float)screenW + 100) - 50.0f;
        float y = 30.0f + (float)(i % 4) * 45.0f;
        float w = 80.0f + (float)(i * 30 % 80);
        float h = 18.0f + (float)(i * 7 % 20);
        float alpha = 0.06f + 0.04f * std::sin(time * 0.5f + i);
        DrawEllipse((int)x, (int)y, (int)w, (int)h, ColorAlpha({50,30,20,255}, alpha));
    }

    // Distant explosions / fires on horizon (LA Ruins only)
    if (zone == ZoneID::LARuins) {
        for (int i = 0; i < 5; ++i) {
            float x = (float)(i * 280 + 80);
            float baseY = (float)(screenH * 2/3 - 20);
            float pulse = 0.5f + 0.5f * std::sin(time * (1.5f + i * 0.3f) + i);
            DrawCircleV({x, baseY}, 18 + pulse * 8, ColorAlpha({255,80,0,255}, 0.12f + pulse*0.08f));
            DrawCircleV({x, baseY}, 10 + pulse * 5, ColorAlpha({255,150,0,255}, 0.20f + pulse*0.10f));
        }
    }
}

// â”€â”€â”€ Skyline silhouette â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Background::drawSkyline(Camera2D cam, int screenW, int screenH, ZoneID zone) const {
    // Horizon Y in screen coordinates
    float horizonY = screenH * 0.62f;
    // Parallax factor (buildings scroll slower than world)
    float parallaxX = cam.target.x * 0.08f;

    Color bldColor = {12, 12, 18, 255};  // default seguro
    switch (zone) {
        case ZoneID::LARuins:        bldColor = {18, 10, 8,  255}; break;
        case ZoneID::Bunker:         bldColor = {10, 18, 10, 255}; break;
        case ZoneID::KronosForge:    bldColor = {22, 10, 5,  255}; break;
        case ZoneID::KronosNexus:    bldColor = {10, 6,  22, 255}; break;
        case ZoneID::Cemetery:       bldColor = {14, 16, 22, 255}; break;
        case ZoneID::CursedFarm:     bldColor = {22, 18, 10, 255}; break;
        case ZoneID::GhostCity:      bldColor = {16, 18, 26, 255}; break;
        case ZoneID::DarkForest:     bldColor = {8,  16, 10, 255}; break;
        case ZoneID::Catacombs:      bldColor = {16, 12, 20, 255}; break;
        case ZoneID::AbandonedManor: bldColor = {18, 10, 22, 255}; break;
        case ZoneID::InfernoZone:    bldColor = {26, 8,  4,  255}; break;
        default: break;
    }

    // Building silhouettes
    struct Bld { float x, w, h; bool hasAntenna; };
    Bld blds[] = {
        {0,   80, 180, true},
        {70,  55, 140, false},
        {130, 90, 220, true},
        {210, 45, 100, false},
        {255, 70, 160, true},
        {330, 100,200, false},
        {430, 60, 130, true},
        {490, 85, 175, false},
        {580, 50, 120, true},
        {640, 110,190, true},
        {760, 65, 150, false},
        {830, 90, 200, false},
        {930, 55, 110, true},
        {990, 80, 170, false},
        {1080,70, 140, true},
        {1160,100,210, false},
    };

    for (auto& b : blds) {
        float bx = fmodf(b.x - parallaxX + screenW * 2, (float)(screenW + 200)) - 100;
        DrawRectangle((int)bx, (int)(horizonY - b.h), (int)b.w, (int)(b.h + 20), bldColor);
        // Windows (some lit orange/cyan)
        for (int wy = 3; wy < (int)(b.h / 14); ++wy) {
            for (int wx = 1; wx < (int)(b.w / 12); ++wx) {
                if ((wx * 7 + wy * 3 + (int)bx) % 5 == 0) {
                    Color wc = (zone == ZoneID::KronosNexus) ?
                               Color{0,80,200,255} : Color{200,120,20,255};
                    DrawRectangle((int)bx + wx*12, (int)(horizonY - b.h) + wy*14, 6, 8, wc);
                }
            }
        }
        // Antenna
        if (b.hasAntenna) {
            DrawRectangle((int)(bx + b.w/2) - 1, (int)(horizonY - b.h - 28), 2, 30, bldColor);
            float pulse = 0.5f + 0.5f * std::sin(time * 2.0f + b.x);
            DrawCircle((int)(bx + b.w/2), (int)(horizonY - b.h - 28), 3,
                       ColorAlpha({255,50,50,255}, pulse));
        }
    }

    // Ground line
    DrawRectangle(0, (int)horizonY, screenW, 3, ColorAlpha(bldColor, 0.5f));
}

// â”€â”€â”€ Environment objects â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Background::drawEnvObjects() const {
    for (const auto& obj : envObjects) {
        switch (obj.type) {
            case EnvObject::Type::Car:      drawCar(obj.pos, obj.rotation, obj.tint); break;
            case EnvObject::Type::Rubble:   drawRubble(obj.pos, obj.scale, obj.tint); break;
            case EnvObject::Type::Lamppost: drawLamppost(obj.pos, obj.hasFire);       break;
            case EnvObject::Type::FirePit:  drawFirePit(obj.pos, obj.animTimer);      break;
            case EnvObject::Type::Crate:    drawCrate(obj.pos, obj.tint);             break;
            case EnvObject::Type::Terminal: drawTerminal(obj.pos, obj.animTimer);     break;
        }
    }
}

void Background::drawCar(Vector2 pos, float rot, Color tint) const {
    // Draw rotated car using DrawRectanglePro
    Rectangle body = {pos.x, pos.y, 52, 24};
    DrawRectanglePro(body, {26, 12}, rot, tint);
    // Cabin
    Rectangle cabin = {pos.x, pos.y - 4, 30, 14};
    DrawRectanglePro(cabin, {15, 7}, rot, ColorAlpha(tint, 0.7f));
    // Wheel marks
    Color dark = ColorAlpha(BLACK, 0.4f);
    DrawCircleSector({pos.x, pos.y}, 6, rot-30, rot+30, 8, dark);
    // Glow from fire if burning
    if ((int)(rot) % 3 == 0) {
        float pulse = 0.5f + 0.5f * std::sin(time * 3.0f + rot);
        DrawCircleV(pos, 20 + pulse * 8, ColorAlpha({255,80,0,255}, 0.08f + pulse*0.05f));
    }
}

void Background::drawRubble(Vector2 pos, float scale, Color tint) const {
    // Rubble pile = several irregular polygons
    float s = scale * 18.0f;
    DrawCircleV(pos, s, tint);
    DrawCircleV({pos.x + s*0.6f, pos.y - s*0.3f}, s*0.65f, ColorAlpha(tint, 0.7f));
    DrawCircleV({pos.x - s*0.5f, pos.y - s*0.2f}, s*0.5f,  ColorAlpha(tint, 0.8f));
    DrawCircleV({pos.x + s*0.2f, pos.y - s*0.6f}, s*0.4f,  ColorAlpha(tint, 0.6f));
    // Concrete chunks
    DrawRectangle((int)(pos.x - s*0.8f), (int)(pos.y - s*0.4f), (int)(s*0.5f), (int)(s*0.3f), ColorAlpha(tint, 0.9f));
    DrawRectangle((int)(pos.x + s*0.3f), (int)(pos.y - s*0.7f), (int)(s*0.4f), (int)(s*0.25f), ColorAlpha(tint, 0.75f));
}

void Background::drawLamppost(Vector2 pos, bool lit) const {
    Color pole = {55, 55, 65, 255};
    // Post
    DrawRectangle((int)pos.x - 2, (int)pos.y - 50, 4, 50, pole);
    // Arm
    DrawRectangle((int)pos.x,     (int)pos.y - 50, 18, 3, pole);
    // Lamp head
    DrawRectangle((int)pos.x + 14,(int)pos.y - 54, 10, 8, {80,80,90,255});
    if (lit) {
        // Orange sodium glow
        DrawCircleV({pos.x + 19, pos.y - 50}, 12, ColorAlpha({255,180,80,255}, 0.15f));
        DrawCircleV({pos.x + 19, pos.y - 50}, 6,  ColorAlpha({255,200,100,255}, 0.4f));
        DrawCircleV({pos.x + 19, pos.y - 50}, 3,  {255,220,150,255});
    } else {
        DrawRectangle((int)pos.x+16,(int)pos.y-53, 6, 6, {30,30,35,255});
    }
}

void Background::drawFirePit(Vector2 pos, float t) const {
    float pulse = 0.5f + 0.5f * std::sin(t * 4.0f);
    float pulse2= 0.5f + 0.5f * std::sin(t * 6.0f + 1.0f);

    // Outer glow
    DrawCircleV(pos, 30 + pulse*12, ColorAlpha({255,60,0,255},  0.06f + pulse*0.04f));
    DrawCircleV(pos, 22 + pulse*8,  ColorAlpha({255,100,0,255}, 0.12f + pulse*0.06f));
    // Base embers
    DrawCircleV(pos, 14, {80,40,20,255});
    // Flame core
    DrawCircleV({pos.x, pos.y - 6 - pulse*4}, 10 + pulse*4,  ColorAlpha({255,120,0,255}, 0.8f));
    DrawCircleV({pos.x, pos.y - 10 - pulse*5}, 7 + pulse2*3, ColorAlpha({255,200,50,255},0.7f));
    DrawCircleV({pos.x, pos.y - 14 - pulse*6}, 4 + pulse*2,  ColorAlpha({255,240,180,255},0.5f));
    // Flickering tip
    DrawCircleV({pos.x + (pulse-0.5f)*6, pos.y - 18 - pulse*8}, 2.5f,
                ColorAlpha(WHITE, 0.5f + pulse*0.3f));
}

void Background::drawCrate(Vector2 pos, Color tint) const {
    DrawRectangle((int)pos.x - 14, (int)pos.y - 14, 28, 28, tint);
    DrawRectangleLinesEx({pos.x-14, pos.y-14, 28, 28}, 1.5f, ColorAlpha(BLACK, 0.4f));
    // Straps
    DrawLine((int)pos.x-14, (int)pos.y, (int)pos.x+14, (int)pos.y, ColorAlpha(BLACK,0.3f));
    DrawLine((int)pos.x,    (int)pos.y-14,(int)pos.x,  (int)pos.y+14,ColorAlpha(BLACK,0.3f));
    // Stencil letter
    DrawText("S", (int)pos.x - 4, (int)pos.y - 8, 14, ColorAlpha(BLACK, 0.35f));
}

void Background::drawTerminal(Vector2 pos, float t) const {
    float pulse = 0.5f + 0.5f * std::sin(t * 2.5f);
    // Base
    DrawRectangle((int)pos.x - 12, (int)pos.y - 30, 24, 34, {30,30,40,255});
    DrawRectangle((int)pos.x - 16, (int)pos.y + 2,  32, 8,  {25,25,35,255});
    // Screen
    DrawRectangle((int)pos.x - 9,  (int)pos.y - 27, 18, 20, {0,10,20,255});
    // Screen glow (red for KRONOS)
    Color sc = {(unsigned char)(160 + (int)(80*pulse)),
                (unsigned char)(10 + (int)(10*pulse)),
                (unsigned char)(10), 255};
    DrawRectangle((int)pos.x - 8, (int)pos.y - 26, 16, 18, ColorAlpha(sc, 0.7f));
    // KRONOS eye on screen
    DrawCircleV({pos.x, pos.y - 17}, 4 + pulse*2, ColorAlpha({255,0,0,255}, 0.7f));
    DrawCircleV({pos.x, pos.y - 17}, 2, RED);
    // Ambient glow
    DrawCircleV({pos.x, pos.y - 17}, 20 + pulse*8, ColorAlpha({255,0,0,255}, 0.04f));
}

// â”€â”€â”€ Ash particles â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

void Background::drawAsh() const {
    for (int i = 0; i < (int)ashParticles.size(); ++i) {
        float alpha = 0.3f + 0.2f * std::sin(time * 0.5f + i);
        float r     = 1.5f + (i % 3) * 0.7f;
        DrawCircleV(ashParticles[i], r, ColorAlpha({200,180,160,255}, alpha));
    }
}

// ─── Parallax stars (screen-space) ──────────────────────────────────────────

void Background::drawParallaxStars(Camera2D cam, int screenW, int screenH) const {
    for (const auto& s : stars) {
        // Parallax: far stars (layer=0) move very little, near stars (layer=1) more
        float parallaxFactor = 0.02f + s.layer * 0.06f;
        float sx = fmodf(s.pos.x - cam.target.x * parallaxFactor + screenW * 4, (float)screenW);
        float sy = s.pos.y;

        // Twinkle
        float twinkle = 0.6f + 0.4f * std::sin(time * (1.5f + s.layer) + s.twinkle);
        float alpha   = s.brightness * twinkle;
        float radius  = 0.8f + s.layer * 1.2f;

        // Draw star with optional glow for bright ones
        if (s.brightness > 0.75f) {
            DrawCircleV({sx, sy}, radius * 2.5f, ColorAlpha(WHITE, alpha * 0.08f));
            DrawCircleV({sx, sy}, radius * 1.5f, ColorAlpha(WHITE, alpha * 0.25f));
        }
        DrawCircleV({sx, sy}, radius, ColorAlpha(WHITE, alpha));
    }
}

// ─── Data stream / Matrix rain (screen-space, for tech zones) ────────────────

void Background::drawDataStream(int /*screenW*/, int /*screenH*/, ZoneID zone) const {
    bool isTech = (zone == ZoneID::KronosForge || zone == ZoneID::KronosNexus ||
                   zone == ZoneID::Bunker);
    if (!isTech) return;

    Color streamCol = (zone == ZoneID::KronosNexus) ? Color{0,180,255,255} :
                      (zone == ZoneID::KronosForge) ? Color{255,80,0,255} :
                                                      Color{0,255,100,255};

    static const char* chars = "01ABCDEF#@$%";
    int charCount = 12;

    for (const auto& d : dataDrops) {
        // Draw a vertical column of characters
        for (int j = 0; j < d.len; ++j) {
            float charY = d.y - j * 12.0f;
            if (charY < 0 || charY > 730.0f) continue;

            float brightness = 1.0f - (float)j / d.len;
            float alpha      = d.alpha * brightness;

            // Head character is brighter/white
            Color c = (j == 0) ? ColorAlpha(WHITE, alpha * 1.2f)
                                : ColorAlpha(streamCol, alpha);

            char ch[2] = {chars[(int)(d.x * 7 + charY * 3 + j * 17) % charCount], 0};
            DrawText(ch, (int)d.x, (int)charY, 11, c);
        }
    }
}
