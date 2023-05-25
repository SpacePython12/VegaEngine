layout (location = 0) in uvec3 ScreenPos;
layout (location = 1) in uvec3 TilePos;
layout (location = 2) in uint PalLine;

out uvec3 OTilePos;
out uint OPalLine;

void main() {
    gl_Position = vec4(ScreenPos.xyz, 1.0);
    OTilePos = TilePos;
    OPalLine = PalLine;
}