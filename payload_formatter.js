function decodeUplink(input) {
    var bytes = input.bytes;
    var data = {};

    if (bytes.length < 4) {
        return { errors: ["Neplatná délka payloadu"] };
    }

    var tempInt = (bytes[0] << 8) | bytes[1];
    if (tempInt & 0x8000) {
        tempInt -= 0x10000;
    }
    data.temperature = tempInt / 100.0;

    var humInt = (bytes[2] << 8) | bytes[3];
    data.humidity = humInt / 100.0;

    return { data: data, warnings: [], errors: [] };
}