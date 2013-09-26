# Generate release for PXDrum
RELEASE_DIR=pxdrum_release

rm -R ${RELEASE_DIR}
mkdir ${RELEASE_DIR}

# executables
cp EBOOT.PBP ${RELEASE_DIR}/EBOOT.PBP
cp PARAM.SFO ${RELEASE_DIR}/PARAM.SFO
cp xdrum ${RELEASE_DIR}/pxdrum

# docs
cp changelog.txt ${RELEASE_DIR}/changelog.txt
cp _readme.txt ${RELEASE_DIR}/_readme.txt
cp _install.txt ${RELEASE_DIR}/_install.txt
cp _quickstart.txt ${RELEASE_DIR}/_quickstart.txt
cp "_quickstart (PSP).txt" "${RELEASE_DIR}/_quickstart (PSP).txt"
cp info_* ${RELEASE_DIR}

# config files
cp joymap.cfg ${RELEASE_DIR}/joymap.cfg
cp joymap.psp.cfg ${RELEASE_DIR}/joymap.psp.cfg
cp joymap.ps3.cfg ${RELEASE_DIR}/joymap.ps3.cfg

# support folders
cp -R -u gfx ${RELEASE_DIR}/gfx
cp -R -u doc ${RELEASE_DIR}/doc
cp -R -u help ${RELEASE_DIR}/help
mkdir ${RELEASE_DIR}/songs
cp -t ${RELEASE_DIR}/songs "songs/disco ex.xds" "songs/disco ex2.xds"
cp -t ${RELEASE_DIR}/songs "songs/funk ex.xds" "songs/funky cold medina.xds"
cp -t ${RELEASE_DIR}/songs "songs/get enuf.xds" "songs/groove1.xds"
cp -t ${RELEASE_DIR}/songs "songs/latin1.xds" "songs/my sharona.xds"
cp -t ${RELEASE_DIR}/songs "songs/reggae ex.xds"
cp -t ${RELEASE_DIR}/songs "songs/rock1.xds" "songs/rock ex.xds"
mkdir ${RELEASE_DIR}/kits
cp -R -u kits/default ${RELEASE_DIR}/kits/default
cp -R -u kits/[KB6]_Alesis_HR16A ${RELEASE_DIR}/kits/[KB6]_Alesis_HR16A
cp -R -u kits/[KB6]_Casio_RZ1 ${RELEASE_DIR}/kits/[KB6]_Casio_RZ1
cp -R -u kits/[KB6]_Korg_DDD-1 ${RELEASE_DIR}/kits/[KB6]_Korg_DDD-1
cp -R -u kits/[KB6]_Korg_ProWave ${RELEASE_DIR}/kits/[KB6]_Korg_ProWave
cp -R -u kits/[KB6]_Linn_LM-1 ${RELEASE_DIR}/kits/[KB6]_Linn_LM-1
cp -R -u kits/[KB6]_Roland_CR78 ${RELEASE_DIR}/kits/[KB6]_Roland_CR78
cp -R -u kits/[KB6]_Roland_CR8000 ${RELEASE_DIR}/kits/[KB6]_Roland_CR8000
cp -R -u kits/[KB6]_Roland_MT32 ${RELEASE_DIR}/kits/[KB6]_Roland_MT32
cp -R -u kits/[KB6]_Roland_TR505 ${RELEASE_DIR}/kits/[KB6]_Roland_TR505
cp -R -u kits/[KB6]_SC_Studio-440 ${RELEASE_DIR}/kits/[KB6]_SC_Studio-440
cp -R -u kits/[KB6]_Yamaha_RX-21 ${RELEASE_DIR}/kits/[KB6]_Yamaha_RX-21
cp -R -u kits/[KB6]_Yamaha_TX16W ${RELEASE_DIR}/kits/[KB6]_Yamaha_TX16W
mkdir ${RELEASE_DIR}/wav

ls -R ${RELEASE_DIR}

