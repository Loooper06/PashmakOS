#include <cstring>
#include "PetRenderer.h"
#include "PetAppearance.h"
#include "../display/Display.h"

namespace
{
    constexpr int16_t SCREEN_W = 320;
    constexpr int16_t SCREEN_H = 240;

    constexpr int16_t HEADER_H = 36;
    constexpr int16_t FOOTER_Y = 194;
    constexpr int16_t FOOTER_H = SCREEN_H - FOOTER_Y;

    constexpr uint16_t BG = 0x1082;
    constexpr uint16_t PANEL = 0x2127;
    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t PURPLE = 0xB81F;
    constexpr uint16_t PINK = 0xF2B5;

    constexpr uint16_t SHADOW = 0x0861;
    constexpr uint16_t FUR = 0xFBAA;
    constexpr uint16_t FUR_LIGHT = 0xFE0F;
    constexpr uint16_t FUR_DARK = 0xD4A4;
    constexpr uint16_t CREAM = 0xFF1A;
    constexpr uint16_t CREAM_DARK = 0xF6B7;
    constexpr uint16_t EAR_PINK = 0xF2B5;
    constexpr uint16_t BLUSH = 0xF1B7;
    constexpr uint16_t EYE = 0x18C3;
    constexpr uint16_t NOSE = 0x3928;
    constexpr uint16_t MOUTH = 0x5129;
    constexpr uint16_t TONGUE = 0xE8B7;

    enum class PetAction : uint8_t { Stand, Walk, Sit, LieDown, Hop };

    struct DogPose
    {
        int8_t bob;
        int8_t leftEarX;
        int8_t rightEarX;
        int8_t legSwing;
        int8_t tailSwing;
        int16_t travelX;
        int8_t gazeX;
        PetAction action;
        bool blinking;
        bool moving;
    };

    DogPose getDogPose(PetMode mode, uint32_t frame)
    {
        DogPose pose{};

        // Walk between two safe positions, pause in a seated pose at each
        // endpoint, then reverse. This makes sitting visible even during Work
        // and Play instead of waiting for the much longer Nap session.
        constexpr int16_t LEFT_CENTER = 112;
        constexpr int16_t RIGHT_CENTER = 208;
        constexpr uint16_t WALK_FRAMES = 52;
        constexpr uint16_t SIT_FRAMES = 36;
        constexpr uint16_t CYCLE_FRAMES = 2U * (WALK_FRAMES + SIT_FRAMES);

        const bool resting = mode == PetMode::Nap;
        pose.travelX = 160;
        pose.action = PetAction::Sit;
        pose.moving = false;

        if (resting)
        {
            // Nap is a calm, centered seated pose with slow breathing.
            const uint32_t breathPhase = frame % 48U;
            pose.bob = (breathPhase < 12U) ? 0
                     : (breathPhase < 24U) ? -1
                     : (breathPhase < 36U) ? -2
                     : -1;
            const uint32_t tailPhase = (frame / 8U) % 8U;
            constexpr int8_t tailCycle[8] = {-2, -1, 0, 1, 2, 1, 0, -1};
            pose.tailSwing = tailCycle[tailPhase];
            const uint32_t lookPhase = (frame / 30U) % 4U;
            pose.gazeX = (lookPhase == 1U) ? -3 : (lookPhase == 3U) ? 3 : 0;
            const uint32_t blinkPhase = frame % 97U;
            pose.blinking = blinkPhase < 3U;
            pose.legSwing = 0;
            const uint32_t earPhase = frame % 83U;
            if (earPhase >= 4U && earPhase < 7U) pose.leftEarX = -2;
            if (earPhase >= 31U && earPhase < 34U) pose.rightEarX = 2;
        }
        else
        {
            const uint32_t cycle = frame % CYCLE_FRAMES;
            const uint32_t forwardEnd = WALK_FRAMES;
            const uint32_t pauseRightEnd = forwardEnd + SIT_FRAMES;
            const uint32_t backwardEnd = pauseRightEnd + WALK_FRAMES;

            if (cycle < forwardEnd)
            {
                const uint32_t t = (cycle * 1000UL) / (WALK_FRAMES - 1U);
                const uint32_t eased = (t * t * (3000UL - 2UL * t)) / 1000000UL;
                pose.travelX = static_cast<int16_t>(
                    LEFT_CENTER + (static_cast<int32_t>(RIGHT_CENTER - LEFT_CENTER) * eased) / 1000);
                if (cycle == WALK_FRAMES - 1U) pose.travelX = RIGHT_CENTER;
                pose.action = PetAction::Walk;
                pose.moving = true;
                pose.gazeX = 6;
            }
            else if (cycle < pauseRightEnd)
            {
                pose.travelX = RIGHT_CENTER;
                pose.action = PetAction::Sit;
                pose.moving = false;
                pose.gazeX = 0;
            }
            else if (cycle < backwardEnd)
            {
                const uint32_t phase = cycle - pauseRightEnd;
                const uint32_t t = (phase * 1000UL) / (WALK_FRAMES - 1U);
                const uint32_t eased = (t * t * (3000UL - 2UL * t)) / 1000000UL;
                pose.travelX = static_cast<int16_t>(
                    RIGHT_CENTER - (static_cast<int32_t>(RIGHT_CENTER - LEFT_CENTER) * eased) / 1000);
                if (phase == WALK_FRAMES - 1U) pose.travelX = LEFT_CENTER;
                pose.action = PetAction::Walk;
                pose.moving = true;
                pose.gazeX = -6;
            }
            else
            {
                pose.travelX = LEFT_CENTER;
                pose.action = PetAction::Sit;
                pose.moving = false;
                pose.gazeX = 0;
            }

            if (pose.moving)
            {
                const uint8_t gait = (frame / 3U) & 3U;
                pose.legSwing = (gait == 0U || gait == 3U) ? -5 : 5;
                pose.tailSwing = (gait < 2U) ? -4 : 4;
                pose.bob = (gait == 0U || gait == 2U) ? -1 : 2;
                pose.blinking = (frame % 53U) < 2U;
            }
            else
            {
                pose.legSwing = 0;
                pose.bob = ((frame / 12U) & 1U) ? 0 : -1;
                pose.tailSwing = ((frame / 10U) & 1U) ? 2 : -2;
                pose.blinking = (frame % 71U) < 3U;
            }

            if (mode == PetMode::Play && pose.moving)
            {
                pose.legSwing = pose.legSwing < 0 ? -7 : 7;
                pose.tailSwing = pose.tailSwing < 0 ? -6 : 6;
            }
        }

        // Small ear motion, identical across all pet skins.
        const uint8_t earPhase = (frame / 5U) % 12U;
        if (!resting)
        {
            if (earPhase >= 2U && earPhase < 5U) pose.leftEarX = -2;
            if (earPhase >= 7U && earPhase < 10U) pose.rightEarX = 2;
        }

        // Play mode has a more energetic but still synchronized gait.
        if (mode == PetMode::Play && !resting)
        {
            pose.legSwing = pose.legSwing < 0 ? -7 : 7;
            pose.tailSwing = pose.tailSwing < 0 ? -6 : 6;
        }

        return pose;
    }
}

PetRenderer::PetRenderer()
    : _initialized(false),
      _footerInitialized(false),
      _lastMode(PetMode::Work),
      _dogInitialized(false),
      _lastDogMode(PetMode::Work),
      _lastDogBob(0),
      _lastDogLeftEarX(0),
      _lastDogRightEarX(0),
      _lastDogMoving(false),
      _lastDogBlinking(false),
      _lastMessage(nullptr),
      _messageInitialized(false)
{
}

void PetRenderer::drawMessage(Display &d, const char *message)
{
    if (message == nullptr)
        message = "";

    if (_messageInitialized &&
        _lastMessage != nullptr &&
        strcmp(_lastMessage, message) == 0)
    {
        return;
    }

    // Header layout: title on the left, status text in the middle,
    // and the decorative icon on the right. Built-in font is 6 px wide.
    constexpr int16_t MESSAGE_X = 142;
    constexpr int16_t MESSAGE_Y = 4;
    constexpr int16_t MESSAGE_W = 142;
    constexpr int16_t MESSAGE_H = 28;
    constexpr int16_t FONT_W = 6;

    d.fillRect(MESSAGE_X, MESSAGE_Y, MESSAGE_W, MESSAGE_H, PANEL);

    // Keep status text within its reserved area, even for long messages.
    constexpr size_t MAX_MESSAGE_CHARS = (MESSAGE_W - 8) / FONT_W;
    char visibleMessage[MAX_MESSAGE_CHARS + 1];
    size_t len = strlen(message);
    if (len > MAX_MESSAGE_CHARS)
    {
        len = MAX_MESSAGE_CHARS;
        if (len > 3)
            len -= 3;
        memcpy(visibleMessage, message, len);
        visibleMessage[len++] = '.';
        visibleMessage[len++] = '.';
        visibleMessage[len++] = '.';
    }
    else
    {
        memcpy(visibleMessage, message, len);
    }
    visibleMessage[len] = '\0';

    const int16_t textWidth = static_cast<int16_t>(len * FONT_W);
    const int16_t textX = MESSAGE_X + MESSAGE_W - 4 - textWidth;
    d.drawText(visibleMessage, textX, 9, 1, WHITE);

    _lastMessage = message;
    _messageInitialized = true;
}

void PetRenderer::drawHeader(Display &d)
{
    d.fillRect(0, 0, SCREEN_W, HEADER_H, PANEL);

    d.drawText("PashmakOS", 12, 10, 2, PURPLE);

    d.fillCircle(294, 18, 8, PINK);
    d.fillCircle(291, 15, 2, WHITE);
}

void PetRenderer::drawDog(Display &d, PetMode mode, uint32_t frame, uint8_t petId)
{
    const bool sleeping = mode == PetMode::Nap;
    const uint16_t fur = petId == 1 ? 0xCE79 : (petId == 2 ? 0x7D7C : FUR);
    const uint16_t furDark = petId == 1 ? 0x9C71 : (petId == 2 ? 0x4A54 : FUR_DARK);
    const uint16_t furLight = petId == 1 ? 0xF7BE : (petId == 2 ? 0xBDF7 : FUR_LIGHT);
    const uint16_t earPink = petId == 1 ? 0xF2B5 : (petId == 2 ? 0x7DFF : EAR_PINK);
    const uint16_t cream = petId == 1 ? 0xFFFF : (petId == 2 ? 0xCE79 : CREAM);
    const bool playing = mode == PetMode::Play;

    const DogPose pose = getDogPose(mode, frame);

    const int16_t cx = 160;
    d.setDogPosition(pose.travelX, 0);
    const int16_t cy = 112 + pose.bob + (pose.action == PetAction::Sit ? 4 : 0) + (pose.action == PetAction::LieDown ? 16 : 0);

    const int16_t lx = pose.leftEarX;
    const int16_t rx = pose.rightEarX;

    d.beginDogFrame(BG);

    d.fillEllipse(cx, 185, pose.action == PetAction::LieDown ? 48 : 43, 5, SHADOW);

    // Each pet owns its body silhouette and pose-specific limb geometry.
    const PetBodyPose bodyPose = static_cast<PetBodyPose>(pose.action);
    if (petId == 0)
        drawPashmakBody(d, bodyPose, pose.legSwing, pose.tailSwing, fur, furDark, furLight, cream);
    else if (petId == 1)
        drawMochiBody(d, bodyPose, pose.legSwing, pose.tailSwing, fur, furDark, furLight, cream);
    else
        drawPixelBody(d, bodyPose, pose.legSwing, pose.tailSwing, fur, furDark, furLight, cream);

    d.fillEllipse(119 + lx, 80, 20, 27, furDark);
    d.fillTriangle(101 + lx, 84, 105 + lx, 57, 130 + lx, 65, furDark);
    d.fillEllipse(201 + rx, 80, 20, 27, furDark);
    d.fillTriangle(190 + rx, 65, 215 + rx, 57, 219 + rx, 84, furDark);

    d.fillEllipse(119 + lx, 80, 17, 24, fur);
    d.fillTriangle(104 + lx, 81, 108 + lx, 61, 128 + lx, 68, fur);
    d.fillEllipse(201 + rx, 80, 17, 24, fur);
    d.fillTriangle(192 + rx, 68, 212 + rx, 61, 216 + rx, 81, fur);

    d.fillEllipse(119 + lx, 79, 11, 16, earPink);
    d.fillTriangle(110 + lx, 78, 112 + lx, 65, 124 + lx, 70, earPink);
    d.fillEllipse(201 + rx, 79, 11, 16, earPink);
    d.fillTriangle(196 + rx, 70, 208 + rx, 65, 210 + rx, 78, earPink);

    d.fillEllipse(116 + lx, 73, 3, 5, CREAM_DARK);
    d.fillEllipse(198 + rx, 73, 3, 5, CREAM_DARK);
    d.fillEllipse(123 + lx, 96, 13, 12, fur);
    d.fillEllipse(197 + rx, 96, 13, 12, fur);

    d.fillEllipse(cx, cy, 43, 43, furDark);
    d.fillEllipse(cx, cy - 2, 41, 41, fur);
    d.fillEllipse(cx, cy - 27, 7, 15, cream);
    d.fillEllipse(cx, cy - 15, 5, 13, cream);
    d.fillEllipse(cx - 23, cy + 15, 22, 19, cream);
    d.fillEllipse(cx + 23, cy + 15, 22, 19, cream);
    d.fillEllipse(cx, cy + 27, 25, 13, cream);

    d.fillTriangle(cx - 12, cy - 37, cx - 5, cy - 29, cx - 17, cy - 27, furLight);
    d.fillTriangle(cx + 3, cy - 38, cx + 12, cy - 29, cx + 1, cy - 27, furLight);
    d.fillEllipse(cx - 16, cy - 13, 7, 2, furDark);
    d.fillEllipse(cx + 16, cy - 13, 7, 2, furDark);

    if (pose.blinking)
    {
        d.drawLine(cx - 23, cy - 3, cx - 17, cy + 1, EYE);
        d.drawLine(cx - 17, cy + 1, cx - 11, cy - 3, EYE);
        d.drawLine(cx + 11, cy - 3, cx + 17, cy + 1, EYE);
        d.drawLine(cx + 17, cy + 1, cx + 23, cy - 3, EYE);
    }
    else
    {
        d.fillEllipse(cx - 17 + pose.gazeX, cy - 3, 10, 13, furDark);
        d.fillEllipse(cx + 17 + pose.gazeX, cy - 3, 10, 13, furDark);
        d.fillEllipse(cx - 17 + pose.gazeX, cy - 4, 8, 11, EYE);
        d.fillEllipse(cx + 17 + pose.gazeX, cy - 4, 8, 11, EYE);
        d.fillEllipse(cx - 17 + pose.gazeX, cy - 3, 5, 8, MOUTH);
        d.fillEllipse(cx + 17 + pose.gazeX, cy - 3, 5, 8, MOUTH);
        d.fillCircle(cx - 19 + pose.gazeX, cy - 8, 3, WHITE);
        d.fillCircle(cx + 15 + pose.gazeX, cy - 8, 3, WHITE);
        d.fillCircle(cx - 14, cy - 1, 1, WHITE);
        d.fillCircle(cx + 20, cy - 1, 1, WHITE);
    }

    // Cheeks bounce subtly with the walking gait instead of staying static.
    const int8_t cheekBob = pose.moving
        ? static_cast<int8_t>(((frame / 3U) & 1U) ? 2 : -1)
        : 0;
    const int8_t cheekPulse = pose.moving && ((frame / 6U) & 1U) ? 1 : 0;
    d.fillEllipse(cx - 29, cy + 13 + cheekBob, 7 + cheekPulse, 4, BLUSH);
    d.fillEllipse(cx + 29, cy + 13 + cheekBob, 7 + cheekPulse, 4, BLUSH);
    d.fillCircle(cx - 31, cy + 11 + cheekBob, 1, cream);
    d.fillCircle(cx + 27, cy + 11 + cheekBob, 1, cream);
    d.fillEllipse(cx - 9, cy + 17, 13, 10, cream);
    d.fillEllipse(cx + 9, cy + 17, 13, 10, cream);
    d.fillTriangle(cx - 5, cy + 10, cx + 5, cy + 10, cx, cy + 16, NOSE);
    d.fillCircle(cx, cy + 11, 3, NOSE);
    d.fillCircle(cx - 1, cy + 10, 1, WHITE);

    // The mouth opens/closes while walking, giving the pet a panting expression.
    // Play mode pants more often; Work mode uses a calmer cadence.
    const uint8_t mouthPhase = static_cast<uint8_t>((frame / (playing ? 3U : 5U)) % 4U);
    const bool mouthOpen = pose.moving && (playing ? mouthPhase != 0U : mouthPhase == 1U || mouthPhase == 2U);

    if (mouthOpen)
    {
        d.drawLine(cx, cy + 15, cx, cy + 21, MOUTH);
        d.fillEllipse(cx, cy + 25, playing ? 8 : 6, playing ? 8 : 6, MOUTH);
        d.fillEllipse(cx, cy + 27, playing ? 5 : 3, playing ? 5 : 3, TONGUE);
        d.drawLine(cx, cy + 23, cx, cy + 27, MOUTH);
    }
    else
    {
        d.drawLine(cx, cy + 16, cx, cy + 21, MOUTH);
        d.drawLine(cx, cy + 21, cx - 5, cy + 18, MOUTH);
        d.drawLine(cx - 5, cy + 18, cx - 8, cy + 18, MOUTH);
        d.drawLine(cx, cy + 21, cx + 5, cy + 18, MOUTH);
        d.drawLine(cx + 5, cy + 18, cx + 8, cy + 18, MOUTH);
    }

    if (playing)
    {
        d.fillCircle(111, 113, 2, cream);
        d.fillCircle(209, 113, 2, cream);
        d.drawLine(112, 103, 112, 109, WHITE);
        d.drawLine(109, 106, 115, 106, WHITE);
        d.drawLine(208, 103, 208, 109, WHITE);
        d.drawLine(205, 106, 211, 106, WHITE);
    }

    if (sleeping)
    {
        d.drawText("Z", 207, 65, 2, WHITE);
        d.drawText("z", 224, 51, 1, WHITE);
    }

    d.endDogFrame();
}

void PetRenderer::drawFooter(Display &d, PetMode mode, uint8_t petId, const char *dateTime)
{
    d.fillRect(0, FOOTER_Y, SCREEN_W, FOOTER_H, PANEL);

    switch (mode)
    {
    case PetMode::Work:
        d.drawText("Focus Time", 100, 197, 2, WHITE);
        break;
    case PetMode::Nap:
        d.drawText("Rest Time", 106, 197, 2, WHITE);
        break;
    case PetMode::Play:
        d.drawText("Move Around", 94, 197, 2, WHITE);
        break;
    }

    const char *petNames[] = {"Pashmak", "Mochi", "Pixel"};
    d.drawText(petId < 3 ? petNames[petId] : "Pashmak", 121, 215, 1, WHITE);
    d.drawText("PashmakOS v0.1", 8, 228, 1, WHITE);
    if (dateTime != nullptr)
        d.drawText(dateTime, 190, 228, 1, WHITE);
}

void PetRenderer::draw(Display &d, PetMode mode, uint32_t frame, uint8_t petId, const char *message, const char *dateTime)
{
    if (!_initialized)
    {
        d.clear(BG);
        drawHeader(d);
        _initialized = true;
    }

    const bool dateTimeChanged = dateTime != nullptr && strcmp(_lastDateTime, dateTime) != 0;
    if (!_footerInitialized || mode != _lastMode || petId != _lastPetId || dateTimeChanged)
    {
        drawFooter(d, mode, petId, dateTime);
        if (dateTime != nullptr)
        {
            strncpy(_lastDateTime, dateTime, sizeof(_lastDateTime) - 1);
            _lastDateTime[sizeof(_lastDateTime) - 1] = '\0';
        }
        _lastMode = mode;
        _lastPetId = petId;
        _footerInitialized = true;
    }

    drawMessage(d, message);

    const DogPose pose = getDogPose(mode, frame);
    const bool dogChanged =
        !_dogInitialized ||
        petId != _lastDogPetId ||
        mode != _lastDogMode ||
        pose.bob != _lastDogBob ||
        pose.travelX != _lastDogX ||
        pose.gazeX != _lastDogGazeX ||
        pose.moving != _lastDogMoving ||
        pose.legSwing != _lastDogLegSwing ||
        pose.tailSwing != _lastDogTailSwing ||
        static_cast<uint8_t>(pose.action) != _lastDogAction ||
        pose.blinking != _lastDogBlinking ||
        pose.leftEarX != _lastDogLeftEarX ||
        pose.rightEarX != _lastDogRightEarX;

    if (!dogChanged)
        return;

    drawDog(d, mode, frame, petId);

    _lastDogMode = mode;
    _lastDogPetId = petId;
    _lastDogBob = pose.bob;
    _lastDogX = pose.travelX;
    _lastDogGazeX = pose.gazeX;
    _lastDogMoving = pose.moving;
    _lastDogLegSwing = pose.legSwing;
    _lastDogTailSwing = pose.tailSwing;
    _lastDogAction = static_cast<uint8_t>(pose.action);
    _lastDogBlinking = pose.blinking;
    _lastDogLeftEarX = pose.leftEarX;
    _lastDogRightEarX = pose.rightEarX;
    _dogInitialized = true;
}
