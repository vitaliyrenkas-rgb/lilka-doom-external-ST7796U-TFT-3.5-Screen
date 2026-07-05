#include <Arduino.h>
#include "lilka.h"
#include "doom_splash.h"
#include "external_ili9488.h"
#include "esp_ota_ops.h"

extern "C" {
#include "i_sound.h"
#include "doomkeys.h"
#include "doomgeneric.h"
#include "d_alloc.h"
#include "doomstat.h"
}

#include "esp_partition.h"
#include "esp_system.h"
#include <SD.h> //for exit flag creation 

// #include "esp_attr.h"

// static constexpr uint32_t DOOM_BOOT_MAGIC = 0xD00D5EC0;
// RTC_NOINIT_ATTR uint32_t doomBootMagic;
// RTC_NOINIT_ATTR uint32_t doomBootMagicInv;

// static void doomSecondStageResetOnce() {
//     const bool armed =
//         (doomBootMagic == DOOM_BOOT_MAGIC) &&
//         (doomBootMagicInv == ~DOOM_BOOT_MAGIC);

//     if (!armed) {
//         doomBootMagic = DOOM_BOOT_MAGIC;
//         doomBootMagicInv = ~DOOM_BOOT_MAGIC;
//         delay(150);
//         esp_restart();
//     }

//     doomBootMagic = 0;
//     doomBootMagicInv = 0;
// }
static constexpr const char* DOOM_LAUNCH_FLAG_1 = "/doom_launch.flag";
static constexpr const char* DOOM_LAUNCH_FLAG_2 = "doom_launch.flag";


static void rebootToKeiraOS() {
    const esp_partition_t* keiraPartition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_0,
        nullptr
    );

    if (keiraPartition != nullptr) {
        esp_ota_set_boot_partition(keiraPartition);
    }

    delay(100);
    esp_restart();
}


static bool removeLaunchFlagIfExists(const char* path) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        return false;
    }

    f.close();
    SD.remove(path);
    return true;
}

static bool consumeKeiraLaunchFlagOrReturn() {
    bool consumed = false;

    consumed = removeLaunchFlagIfExists(DOOM_LAUNCH_FLAG_1);
    if (!consumed) {
        consumed = removeLaunchFlagIfExists(DOOM_LAUNCH_FLAG_2);
    }

    if (!consumed) {
        rebootToKeiraOS();
        return false;
    }

    return true;
}


extern void doomgeneric_Create(int argc, char** argv);
extern void doomgeneric_Tick();

typedef struct {
    uint8_t key;
    bool pressed;
} doomkey_t;

doomkey_t keyqueue[16];
uint16_t keyqueueRead = 0;
uint16_t keyqueueWrite = 0;
uint64_t lastRender = 0;

SemaphoreHandle_t inputMutex;
SemaphoreHandle_t backBufferMutex;
EventGroupHandle_t backBufferEvent;
TaskHandle_t gameTaskHandle;
TaskHandle_t drawTaskHandle;

uint32_t* backBuffer = NULL;

sound_module_t DG_sound_module;
extern sound_module_t sound_module_I2S;
extern sound_module_t sound_module_Buzzer;
extern sound_module_t sound_module_NoSound;
int use_libsamplerate = 0;
float libsamplerate_scale = 0.65f;

void gameTask(void* arg);
void drawTask(void* arg);

char nextWeaponKey = '2';

volatile bool startHeld = false;
volatile bool selectHeld = false;

//Diagnostic patch (100 Frames or 1000ms divider) - counting parameters
#define DOOM_FPS_SD_LOG 1

#if DOOM_FPS_SD_LOG
static const char* DOOM_FPS_LOG_PATH = "/doom_fps_diag.csv";

static uint32_t fpsLogStartMs = 0;
static uint32_t fpsLogFrames = 0;

static uint64_t fpsWaitUs = 0;
static uint64_t fpsPresentUs = 0;
static uint64_t fpsHudUs = 0;
static uint64_t fpsDrawUs = 0;

static uint32_t fpsMaxWaitUs = 0;
static uint32_t fpsMaxPresentUs = 0;
static uint32_t fpsMaxDrawUs = 0;

static void doomFpsLogInit() {
    SD.remove(DOOM_FPS_LOG_PATH);

    File f = SD.open(DOOM_FPS_LOG_PATH, FILE_WRITE);
    if (f) {
        f.println("ms,window_ms,frames,fps,avg_wait_us,avg_present_us,avg_hud_us,avg_draw_us,max_wait_us,max_present_us,max_draw_us");
        f.close();
    }

    fpsLogStartMs = millis();
}

static void doomFpsLogFrame(uint32_t waitUs, uint32_t presentUs, uint32_t hudUs, uint32_t drawUs) {
    uint32_t nowMs = millis();

    if (fpsLogStartMs == 0) {
        fpsLogStartMs = nowMs;
    }

    fpsLogFrames++;

    fpsWaitUs += waitUs;
    fpsPresentUs += presentUs;
    fpsHudUs += hudUs;
    fpsDrawUs += drawUs;

    if (waitUs > fpsMaxWaitUs) fpsMaxWaitUs = waitUs;
    if (presentUs > fpsMaxPresentUs) fpsMaxPresentUs = presentUs;
    if (drawUs > fpsMaxDrawUs) fpsMaxDrawUs = drawUs;

    uint32_t windowMs = nowMs - fpsLogStartMs;

    if (fpsLogFrames < 100 && windowMs < 1000) {
        return;
    }

    uint32_t fps100 = windowMs ? (uint32_t)(((uint64_t)fpsLogFrames * 100000ULL) / windowMs) : 0;

    File f = SD.open(DOOM_FPS_LOG_PATH, FILE_APPEND);
    if (f) {
        f.printf("%lu,%lu,%lu,%lu.%02lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                 (unsigned long)nowMs,
                 (unsigned long)windowMs,
                 (unsigned long)fpsLogFrames,
                 (unsigned long)(fps100 / 100),
                 (unsigned long)(fps100 % 100),
                 (unsigned long)(fpsWaitUs / fpsLogFrames),
                 (unsigned long)(fpsPresentUs / fpsLogFrames),
                 (unsigned long)(fpsHudUs / fpsLogFrames),
                 (unsigned long)(fpsDrawUs / fpsLogFrames),
                 (unsigned long)fpsMaxWaitUs,
                 (unsigned long)fpsMaxPresentUs,
                 (unsigned long)fpsMaxDrawUs);
        f.close();
    }

    fpsLogStartMs = nowMs;
    fpsLogFrames = 0;

    fpsWaitUs = 0;
    fpsPresentUs = 0;
    fpsHudUs = 0;
    fpsDrawUs = 0;

    fpsMaxWaitUs = 0;
    fpsMaxPresentUs = 0;
    fpsMaxDrawUs = 0;
}
#endif
//

void buttonHandler(lilka::Button button, bool pressed) {
    
    xSemaphoreTake(inputMutex, portMAX_DELAY);
    doomkey_t* key = &keyqueue[keyqueueWrite];
    switch (button) {
        case lilka::Button::UP:
            key->key = KEY_UPARROW;
            break;
        case lilka::Button::DOWN:
            key->key = KEY_DOWNARROW;
            break;
        case lilka::Button::LEFT:
            key->key = KEY_LEFTARROW;
            break;
        case lilka::Button::RIGHT:
            key->key = KEY_RIGHTARROW;
            break;
        // No strafing
        case lilka::Button::A:
            key->key = KEY_FIRE;
            break;
       case lilka::Button::B:
            key->key = KEY_USE;
            break;
        case lilka::Button::C:
            key->key = KEY_TAB;
            break;
        case lilka::Button::D:
            // Cycle weapons
            key->key = nextWeaponKey;
            break;
        // Strafing experiment
        // case lilka::Button::A:
        //     key->key = KEY_STRAFE_R;
        //     break;
        // case lilka::Button::B:
        //     key->key = KEY_FIRE;
        //     break;
        // case lilka::Button::C:
        //     key->key = KEY_USE;
        //     break;
        // case lilka::Button::D:
        //     key->key = KEY_STRAFE_L;
        //     break;
        case lilka::Button::SELECT:
            key->key = KEY_ESCAPE;
            break;
        case lilka::Button::START:
            key->key = KEY_ENTER;
            break;
        default:
            // TODO: Log warning?
            xSemaphoreGive(inputMutex);
            return;
    }

    key->pressed = pressed;
    keyqueueWrite = (keyqueueWrite + 1) % 16;
    xSemaphoreGive(inputMutex);
}

void setup() {

    esp_ota_mark_app_valid_cancel_rollback();

    Serial.begin(115200);
    delay(100);
    Serial.println("[DOOM BOOT] setup enter");
    lilka::display.setSplash(doom_splash);
    lilka::begin();
    
    if (!consumeKeiraLaunchFlagOrReturn()) {
    return;
    }

    Serial.printf(
        "[DOOM MEM 1] after lilka::begin heap=%u minHeap=%u maxAlloc=%u psram=%u minPsram=%u\n",
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getFreePsram(),
        ESP.getMinFreePsram()
    );

    Serial.println("[DOOM TFT] externalTftBegin enter");
    externalTftBegin();
    Serial.printf(
        "[DOOM MEM 2] after externalTftBegin heap=%u minHeap=%u maxAlloc=%u psram=%u minPsram=%u\n",
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getFreePsram(),
        ESP.getMinFreePsram()
    );

    inputMutex = xSemaphoreCreateMutex();
    xSemaphoreGive(inputMutex);
    backBufferMutex = xSemaphoreCreateMutex();
    xSemaphoreGive(backBufferMutex);
    backBufferEvent = xEventGroupCreate();
    xEventGroupClearBits(backBufferEvent, 1);

    int argc = 3;
    char arg[] = "doomgeneric";
    char arg2[] = "-iwad";
    char arg3[64];

    // Get firmware arg
    String firmwareFile = lilka::multiboot.getFirmwarePath();
    Serial.printf("[DOOM MULTIBOOT] firmwareFile=%s\n", firmwareFile.c_str());
    lilka::serial_log("Firmware file: %s", firmwareFile.c_str());
    String firmwareDir;
    if (firmwareFile.length()) {
        // Get directory from firmware file
        int lastSlash = firmwareFile.lastIndexOf('/');
        firmwareDir = firmwareFile.substring(0, lastSlash);
        if (firmwareDir.length() == 0) {
            firmwareDir = "/";
        }
    } else {
        firmwareDir = "/";
    }

    bool found = false;
    // Find the WAD file
    Serial.printf("[DOOM WAD] searching in dir=%s\n", firmwareDir.c_str());
    File root = SD.open(firmwareDir.c_str());
    File file;
    while ((file = root.openNextFile())) {
        if (file.isDirectory()) {
            file.close();
            continue;
        }
        String name(file.name());
        name.toLowerCase();
        lilka::serial_log("Checking file: %s", name.c_str());
        if (name.startsWith("doom") && name.endsWith(".wad")) {
            if (firmwareDir.endsWith("/")) {
                firmwareDir = firmwareDir.substring(0, firmwareDir.length() - 1);
            }
            strcpy(arg3, (lilka::fileutils.getSDRoot() + firmwareDir + "/" + file.name()).c_str());
            Serial.printf("[DOOM WAD] found=%s\n", arg3);
            lilka::serial_log("Found .WAD file: %s\n", arg3);
            found = true;
            file.close();
            break;
        }
        file.close();
    }
    root.close();
    if (!found) {
        lilka::Alert alert("Doom", "Не знайдено .WAD-файлу на картці пам'яті");
        alert.draw(&lilka::display);
        while (!alert.isFinished()) {
            alert.update();
        }
        rebootToKeiraOS();
    }
    char* argv[3] = {arg, arg2, arg3};

    // Select sound device
    lilka::Menu soundMenu("Звуковий пристрій");
    soundMenu.addItem("I2S DAC");
    soundMenu.addItem("П'єзо-динамік");
    soundMenu.addItem("Без звуку");
    lilka::Canvas canvas;
    while (!soundMenu.isFinished()) {
        soundMenu.update();
        soundMenu.draw(&canvas);
        lilka::display.drawCanvas(&canvas);
    }
    int soundDevice = soundMenu.getCursor();

    if (soundDevice == 0) {
        // I2S DAC
        DG_sound_module = sound_module_I2S;
    } else if (soundDevice == 1) {
        // Buzzer
        DG_sound_module = sound_module_Buzzer;
    } else {
        // No sound
        DG_sound_module = sound_module_NoSound;
    }

    lilka::display.fillScreen(lilka::colors::Black);

    DG_printf("Doomgeneric starting, WAD file: %s", arg3);

    Serial.println("[DOOM ALLOC] D_AllocBuffers enter");
    D_AllocBuffers();
    Serial.printf(
        "[DOOM MEM 3] after D_AllocBuffers heap=%u minHeap=%u maxAlloc=%u psram=%u minPsram=%u\n",
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getFreePsram(),
        ESP.getMinFreePsram()
    );
    // Back buffer must be allocated before doomgeneric_Create since it calls DG_DrawFrame
    backBuffer = static_cast<uint32_t*>(malloc(DOOMGENERIC_FRAMEBUFFER_BYTES));
    Serial.printf(
        "[DOOM ALLOC] backBuffer=%p bytes=%u\n", backBuffer, (unsigned)DOOMGENERIC_FRAMEBUFFER_BYTES
    );
    Serial.println("[DOOM CREATE] doomgeneric_Create enter");
    doomgeneric_Create(argc, argv);
    Serial.printf(
        "[DOOM MEM 4] after doomgeneric_Create heap=%u minHeap=%u maxAlloc=%u psram=%u minPsram=%u\n",
        ESP.getFreeHeap(),
        ESP.getMinFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getFreePsram(),
        ESP.getMinFreePsram()
    );
    if (backBuffer == NULL) {
        DG_printf("Failed to allocate back buffer\n");
        rebootToKeiraOS();
    }

    // Lilka v2 SELECT is GPIO0/BOOT. Re-apply runtime input mode after init.
    //Restoration of SELECT button ESC MENU function.
    pinMode(LILKA_GPIO_SELECT, INPUT_PULLUP);
    lilka::controller.setGlobalHandler(buttonHandler);

    //Diagnostic patch
    #if DOOM_FPS_SD_LOG
     doomFpsLogInit();
    #endif
    //

    // while (1) {
    //     doomgeneric_Tick();
    // }

    Serial.println("[DOOM RUN] Ready, starting tasks");

    xTaskCreatePinnedToCore(gameTask, "gameTask", 32768, NULL, 1, &gameTaskHandle, 0);
    xTaskCreatePinnedToCore(drawTask, "drawTask", 32768, NULL, 1, &drawTaskHandle, 1);

    while (1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // D_FreeBuffers(); // TODO - never reached
}

void gameTask(void* arg) {
    while (1) {
        doomgeneric_Tick();

        // Print free memory
        // Serial.print("Free heap: ");
        // Serial.print(ESP.getFreeHeap());

        // Print free stack
        // Serial.print("  |  Game task free stack: ");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));

        if (playeringame[consoleplayer]) {
            // We have a player (TODO: might be demo)
            const player_t* plyr = &players[consoleplayer];
            const weapontype_t weapons[NUMWEAPONS] = {
                wp_fist,
                wp_chainsaw,
                wp_pistol,
                wp_shotgun,
                wp_supershotgun,
                wp_chaingun,
                wp_missile,
                wp_plasma,
                wp_bfg,
            };
            const int weaponKeys[NUMWEAPONS] = {'1', '1', '2', '3', '3', '4', '5', '6', '7'};
            int currentWeaponIndex;
            for (int i = 0; i < NUMWEAPONS; i++) {
                if (plyr->readyweapon == weapons[i]) {
                    currentWeaponIndex = i;
                    break;
                }
            }
            nextWeaponKey = weaponKeys[plyr->readyweapon];
            for (int i = 1; i < NUMWEAPONS; i++) {
                int candidate = (currentWeaponIndex + i) % NUMWEAPONS;
                if (plyr->weaponowned[weapons[candidate]] && plyr->ammo[weaponinfo[weapons[candidate]].ammo]) {
                    nextWeaponKey = weaponKeys[candidate];
                    break;
                }
            }
            // Print player position
            // Serial.printf(
            //     "Player health: %d, armor: %d, ammo: %d\r\n",
            //     plyr->health,
            //     plyr->armorpoints,
            //     plyr->ammo[weaponinfo[plyr->readyweapon].ammo]
            // );
            // if (plyr->mo) {
            //     Serial.printf("Player position: %d, %d, %d\r\n", plyr->mo->x, plyr->mo->y, plyr->mo->z);
            // }
        }

        taskYIELD();
    }
}

void drawTask(void* arg) {
    while (1) {
        // Wait for buffer to be ready
       #if DOOM_FPS_SD_LOG
            uint32_t waitStartUs = micros();
        #endif

        xEventGroupWaitBits(backBufferEvent, 1, pdTRUE, pdTRUE, portMAX_DELAY);

        #if DOOM_FPS_SD_LOG
            uint32_t waitDoneUs = micros();
        #endif

        xSemaphoreTake(backBufferMutex, portMAX_DELAY);

        // Calculate FPS
        uint64_t now = millis();
        uint64_t delta = now - lastRender;
        lastRender = now;

        // Main Doom picture goes to the external ILI9488 TFT.
        // Doom native frame: 320x200 -> external TFT: 480x300 + black bars.
        #if DOOM_FPS_SD_LOG
            uint32_t presentStartUs = micros();
        #endif

        externalTftPresentDoomFrame(backBuffer);

        #if DOOM_FPS_SD_LOG
            uint32_t presentDoneUs = micros();
        #endif

        // Keep the stock Lilka display as cheap telemetry/debug HUD.
        lilka::display.setTextBound(0, 0, LILKA_DISPLAY_WIDTH, LILKA_DISPLAY_HEIGHT);
        lilka::display.setTextColor(lilka::colors::White, lilka::colors::Black);
        lilka::display.fillRect(0, 0, 140, 36, lilka::colors::Black);
        lilka::display.setCursor(0, 12);
        lilka::display.setFont(FONT_6x12);
        lilka::display.print("DOOM TFT");
        lilka::display.setCursor(0, 28);
        lilka::display.print("FPS: ");
        lilka::display.print(delta ? (1000 / delta) : 0);

        #if DOOM_FPS_SD_LOG
            uint32_t hudDoneUs = micros();

            uint32_t waitUs = waitDoneUs - waitStartUs;
            uint32_t presentUs = presentDoneUs - presentStartUs;
            uint32_t hudUs = hudDoneUs - presentDoneUs;
            uint32_t drawUs = hudDoneUs - waitDoneUs;
        #endif

        xSemaphoreGive(backBufferMutex);

        #if DOOM_FPS_SD_LOG
            doomFpsLogFrame(waitUs, presentUs, hudUs, drawUs);
        #endif
        taskYIELD();
    }
}

extern "C" void DG_Init() {
}

extern "C" void DG_DrawFrame() {
    // Frame is ready.
    // Acquire back buffer, swap buffers and set event
    xSemaphoreTake(backBufferMutex, portMAX_DELAY);
    uint32_t* temp = backBuffer;
    backBuffer = DG_ScreenBuffer;
    DG_ScreenBuffer = temp;
    xEventGroupSetBits(backBufferEvent, 1);
    xSemaphoreGive(backBufferMutex);
}

extern "C" void DG_SetWindowTitle(const char* title) {
    Serial.print("DG: window title: ");
    Serial.println(title);
}

extern "C" void DG_SleepMs(uint32_t ms) {
    delay(ms);
}

extern "C" uint32_t DG_GetTicksMs() {
    return millis();
}

extern "C" int DG_GetKey(int* pressed, unsigned char* doomKey) {
    xSemaphoreTake(inputMutex, portMAX_DELAY);
    int ret;
    if (keyqueueRead != keyqueueWrite) {
        const doomkey_t* key = &keyqueue[keyqueueRead];
        printf("Got key: %d, pressed: %d\n", key->key, key->pressed);
        *pressed = key->pressed;
        *doomKey = key->key;
        keyqueueRead = (keyqueueRead + 1) % 16;
        ret = 1;
    } else {
        ret = 0;
    }
    xSemaphoreGive(inputMutex);
    return ret;
}

bool hadNewLine = true;

extern "C" void DG_printf(const char* format, ...) {
    // Save string to buffer
    xSemaphoreTake(backBufferMutex, portMAX_DELAY);
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("[DG log] %s", buffer);
    int bottom = 280 / 2 + 150 / 2;
    lilka::display.setFont(u8g2_font_6x12_t_cyrillic);
    if (hadNewLine) {
        hadNewLine = false;
        lilka::display.fillRect(0, bottom, 240, 280 - bottom, lilka::colors::Black);
        lilka::display.setCursor(0, bottom + 10);
    }
    lilka::display.setTextBound(0, bottom, 240, 280 - bottom);
    lilka::display.print(buffer);
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == '\n') {
            hadNewLine = true;
        }
    }
    xSemaphoreGive(backBufferMutex);
}

void loop() {
}
