#include <Katakachayulani.h>
#include <Kazarzeth.h>
#include <daemon.h>

inherit "/std/daemon";

void configure() {
    ::configure();
}

int thought_believe(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_concentrate(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_disbelieve(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_feel(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_imagine(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_subvocalize(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_think(object katakachayulani, mixed what) {
    int notifies;
    switch(typeof(what)) {
    case T_POINTER :
        switch(typeof(what[0])) {
        case T_STRING:
            switch(what[0]) {
            case "taking" :
                mixed array kaz = deep_scan(what[1], (: objectp($1) && begins_with(blueprint($1), Kazarzeth) :));
                Debug_To("elronuan", kaz);
                if(sizeof(kaz)) {
                    notifies += sizeof(kaz);
                    katakachayulani->notify_kazarzeth(kaz);
                }
                break;
            case "putting" :
                break;
            }
            break;
        }
        break;
    }
}

int thought_visualize(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought_will(object katakachayulani, mixed what) {
    int notifies;
    return notifies;
}

int thought(object katakachayulani, mixed what, int type) {
    if(type > Thought_Types_All) {
        error("Invalid Thought_Type: " + printable(type));
        return 0;
    }
    int effects = 0;
    if(type & Thought_Type_Believe)
        effects += thought_believe(katakachayulani, what);
    if(type & Thought_Type_Concentrate)
        effects += thought_concentrate(katakachayulani, what);
    if(type & Thought_Type_Disbelieve)
        effects += thought_disbelieve(katakachayulani, what);
    if(type & Thought_Type_Feel)
        effects += thought_feel(katakachayulani, what);
    if(type & Thought_Type_Imagine)
        effects += thought_imagine(katakachayulani, what);
    if(type & Thought_Type_Subvocalize)
        effects += thought_subvocalize(katakachayulani, what);
    if(type & Thought_Type_Think)
        effects += thought_think(katakachayulani, what);
    if(type & Thought_Type_Visualize)
        effects += thought_visualize(katakachayulani, what);
    if(type & Thought_Type_Will)
        effects += thought_will(katakachayulani, what);
    return effects;
}
