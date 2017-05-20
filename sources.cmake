
set(TRIGGER_HEADERS
   src/include/app.h
   src/include/audio.h
   src/include/bulletlink.h
   src/include/bulletdebugdraw.h
   src/include/codriver.h
   src/include/exception.h
   src/include/hiscore1.h
   src/include/main.h
   src/include/menu.h
   src/include/pengine.h
   src/include/psim.h
   src/include/render.h
   src/include/subsys.h
   src/include/terrainmap.h
   src/include/tinyxml2.h
   src/include/vbuffer.h
   src/include/vehicle.h
   src/include/vmath.h
)

set(TRIGGER_LIB_SOURCES
   src/Trigger/game.cpp
   src/Trigger/menu.cpp
   src/Trigger/render.cpp
)

set(PENGINE_LIB_SOURCES
   src/PEngine/app.cpp
   src/PEngine/audio.cpp
   src/PEngine/fxman.cpp
   src/PEngine/model.cpp
   src/PEngine/physfs_rw.cpp
   src/PEngine/render.cpp
   src/PEngine/terrain.cpp
   src/PEngine/texture.cpp
   src/PEngine/util.cpp
   src/PEngine/vbuffer.cpp
   src/PEngine/vmath.cpp
)

set(PSIM_LIB_SOURCES
   src/PSim/bulletdebugdraw.cpp
   src/PSim/bulletlink.cpp
   src/PSim/sim.cpp
   src/PSim/vehicle.cpp
)

set(TINYXML2_LIB_SOURCES
   src/TinyXML2/tinyxml2.cpp
)

set(TRIGGER_SOURCES
   src/Trigger/main.cpp
)

