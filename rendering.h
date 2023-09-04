struct Rect {
    int x, y, w, h;
};

struct Point {
    int x, y;
};

Rect getClientRect();

/**
 * @param rect The rect to fill
 * @param color Each byte represents red, green, blue, alpha(transparency). e.g. 0xAABBCC30
*/
void fillRect(Rect rect, short color);

/**
 * @param rect The rect to fill
 * @param color Each byte represents red, green, blue, alpha(transparency). e.g. 0xAABBCC30
*/
void strokeRect(Rect rect, short color);

/**
 * @param src The source rect of the bitmap
 * @param dest The destination point of the bitmap
*/
void drawBitmap(Rect src, Point dest);
