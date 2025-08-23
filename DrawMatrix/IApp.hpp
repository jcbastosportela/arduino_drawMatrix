/**
 * ------------------------------------------------------------------------------------------------------------------- *
 *            DrawMatrix                                                                                               *
 * @file      IApp.hpp                                                                                                 *
 * @brief     Interface for main application logic in DrawMatrix system                                              *
 * @date      Sun Jul 27 2025                                                                                          *
 * @author    Joao Carlos Bastos Portela (jcbastosportela@gmail.com)                                                   *
 * @copyright 2025 - 2025, Joao Carlos Bastos Portela                                                                  *
 *            MIT License                                                                                              *
 * ------------------------------------------------------------------------------------------------------------------- *
 */
#ifndef DRAWMATRIX_IAPP
#define DRAWMATRIX_IAPP


/**
 * @class IApp
 * @brief Interface for the main application logic in DrawMatrix system.
 */
class IApp
{
public:
    /**
     * @brief Run the main application loop.
     */
    virtual void run() = 0;
};

#endif   /* DRAWMATRIX_IAPP */
