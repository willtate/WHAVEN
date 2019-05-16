    //New movesprite using getzrange.  Note that I made the getzrange
    //parameters global (&globhiz,&globhihit,&globloz,&globlohit) so they
    //don't need to be passed everywhere.  Also this should make this
    //movesprite function compatible with the older movesprite functions.
int movesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype) {

    long daz, zoffs, templong;
    short retval, dasectnum, tempshort;
    struct spritetype *spr;

    spr = &sprite[spritenum];

    if ((spr->cstat&128) == 0)
        zoffs = -((tilesizy[spr->picnum]*spr->yrepeat)<<1);
    else
        zoffs = 0;

    dasectnum = spr->sectnum;  //Can't modify sprite sectors directly becuase of linked lists
    daz = spr->z+zoffs;  //Must do this if not using the new centered centering (of course)
    retval = clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
                            ((long)spr->clipdist)<<2,ceildist,flordist,cliptype);

    if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
        changespritesect(spritenum,dasectnum);

        //Set the blocking bit to 0 temporarly so getzrange doesn't pick up
        //its own sprite
    tempshort = spr->cstat; spr->cstat &= ~1;
    getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,
                 &globhiz,&globhihit,&globloz,&globlohit,
                 ((long)spr->clipdist)<<2,cliptype);
    spr->cstat = tempshort;

    daz = spr->z+zoffs + dz;
    if ((daz <= globhiz) || (daz > globloz))
    {
        if (retval != 0) return(retval);
        return(16384+dasectnum);
    }
    spr->z = daz-zoffs;
    return(retval);
}
