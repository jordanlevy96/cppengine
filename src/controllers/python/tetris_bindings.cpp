#include <pybind11/pybind11.h>

#include "Tetris.h"

PYBIND11_MODULE(tetris, m)
{
    m.def("CreateTetrimino", &Tetris::CreateTetrimino);
    m.def("GetTetriminoChildMap", &Tetris::GetTetriminoChildMap);
    m.def("RotateTetrimino", &Tetris::RotateTetrimino);
    m.def("MoveTetrimino", &Tetris::MoveTetrimino);
    m.def("TweenTetrimino", &Tetris::TweenTetrimino);
    m.def("CheckRotation", &Tetris::CheckRotation);
    m.def("TetriminoFinishedMovement", &Tetris::TetriminoFinishedMovement);
    m.def("GetTetriminoLoc", &Tetris::GetTetriminoLoc);
}