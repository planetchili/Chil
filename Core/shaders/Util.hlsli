uint2 UnpackUint16(uint value)
{
    uint low = value & 0xFFFF; // Mask to get the lower 16 bits
    uint high = value >> 16; // Shift right 16 bits to get the upper 16 bits
    return uint2(low, high);
}

uint2 UnpackUint8(uint value)
{
    uint low = value & 0xFF; // Mask to get the lower 8 bits
    uint high = (value >> 8) & 0xFF; // Shift right 8 bits to get the upper 8 bits and mask just in case value exceeds 16-bit
    return uint2(low, high);
}