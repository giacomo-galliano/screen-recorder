#ifndef SCREEN_RECORDER_COORDINATES_H
#define SCREEN_RECORDER_COORDINATES_H

class Point {
private:
    double x, y;
public:
    Point();
    Point(double x, double y);
};

class Coordinates : public Point{
private:
    Point p1;
    Point p2;
public:
    Coordinates();
    Coordinates(Point p1, Point p2);
};


#endif //SCREEN_RECORDER_COORDINATES_H
