uniform sampler2D Palette; // GL_RGBA5551
uniform usampler2D Tiles; // GL_R8 

out vec4 FragColor;

in uint PalLine;
in uvec3 TilePos;

void main() {
    uint Index = texture(Tiles, uvec2(TilePos.x, TilePos.y+(TilePos.z * 8)));
    FragColor = texture(Palette, uvec2(Index, PalLine));
}