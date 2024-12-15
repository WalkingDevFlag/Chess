// color.h

#ifndef COLOUR_H

#define COLOUR_H

// macros

#define COLOUR_IS_OK(color)    (((color)&~1)==0)

#define COLOUR_IS_WHITE(color) ((color)==0)
#define COLOUR_IS_BLACK(color) ((color)!=0)

#define COLOUR_IS(piece,color) (FLAG_IS((piece),COLOUR_FLAG(color)))
#define FLAG_IS(piece,flag)     (((piece)&(flag))!=0)

#define COLOUR_OPP(color)      ((color)^(0^1))
#define COLOUR_FLAG(color)     ((color)+1)

#endif // !defined COLOUR_H

// end of color.h
