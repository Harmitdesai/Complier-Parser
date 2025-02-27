#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "execute.h"
#include "lexer.h"

LexicalAnalyzer lexer;

InstructionNode *start = (InstructionNode *)malloc(sizeof(InstructionNode)); //////Start node indicating first instruction

InstructionNode *current = start;

std::unordered_map<std::string, int> varAddr;

/////////Declaring functions////

void parse_numList();
void parse_inputs();
std::string parse_primary();
InstructionNode *parse_defaultCase(InstructionNode *noop);
InstructionNode *parse_case(InstructionNode *noop, std::string id);
InstructionNode *parse_caseList(InstructionNode *noop, std::string id);
InstructionNode *parse_forStmnt();
InstructionNode *parse_switchStmnt();
ConditionalOperatorType parse_relop();
void parse_condition(InstructionNode *condNode);
InstructionNode *parse_ifStmnt();
InstructionNode *parse_whileStmnt();
InstructionNode *parse_inputStmnt();
InstructionNode *parse_outputStmnt();
ArithmeticOperatorType parse_op();
void parse_expr(InstructionNode *instNode);
InstructionNode *parse_assignStmnt();
InstructionNode *parse_stmnt();
InstructionNode *parse_stmntList();
InstructionNode *parse_body();
void parse_idList();
void parse_varSection();
void parse_program();

/////////Writing Parser/////////

Token expect(TokenType t)
{
    Token token = lexer.peek(1);
    if (token.token_type == t)
    {
        return lexer.GetToken();
    }
    return token;
}

void parse_numList()
{
    Token t = expect(NUM);

    inputs.push_back(std::stoi(t.lexeme));

    if (lexer.peek(1).token_type == NUM)
    {
        parse_numList();
    }
}

void parse_inputs()
{
    parse_numList();
}

std::string parse_primary()
{
    if (lexer.peek(1).token_type == ID)
    {
        return expect(ID).lexeme;
    }
    else
    {

        if (varAddr.find(lexer.peek(1).lexeme) != varAddr.end())
        {
            // Key exists
        }
        else
        {
            // Key does not exist
            varAddr[lexer.peek(1).lexeme] = next_available;
            mem[next_available] = std::stoi(lexer.peek(1).lexeme);
            next_available = next_available + 1;
        }

        return expect(NUM).lexeme;
    }
}

InstructionNode *parse_defaultCase(InstructionNode *noop)
{
    InstructionNode *firstInt = nullptr;
    InstructionNode *temp = nullptr;
    expect(DEFAULT);
    expect(COLON);
    firstInt = parse_body();
    temp = firstInt;
    while (temp->next != nullptr)
    {
        temp = temp->next;
    }

    temp->next = noop;

    return firstInt;
}

InstructionNode *parse_case(InstructionNode *noop, std::string id)
{
    InstructionNode *firstInt = new InstructionNode();
    InstructionNode *temp = nullptr;
    firstInt->type = CJMP;
    firstInt->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
    firstInt->cjmp_inst.opernd1_index = varAddr[id];
    expect(CASE);
    firstInt->cjmp_inst.opernd2_index = varAddr[parse_primary()];
    expect(COLON);
    firstInt->cjmp_inst.target = parse_body();
    firstInt->next = noop;
    temp = firstInt->cjmp_inst.target;
    while (temp->next != nullptr)
    {
        temp = temp->next;
    }
    temp->next = noop;
    return firstInt;
}

InstructionNode *parse_caseList(InstructionNode *noop, std::string id)
{
    InstructionNode *firstInt = nullptr;
    firstInt = parse_case(noop, id);
    firstInt->next = noop;
    if (lexer.peek(1).token_type == CASE)
    {
        firstInt->next = parse_caseList(noop, id);
    }

    return firstInt;
}

InstructionNode *parse_forStmnt()
{
    InstructionNode *firstInt = nullptr;

    InstructionNode *whileInt = new InstructionNode();
    InstructionNode *BodyNode = nullptr;

    InstructionNode *jump = new InstructionNode();

    InstructionNode *noop = new InstructionNode();
    InstructionNode *temp = nullptr;

    InstructionNode *sndAssign = nullptr;

    expect(FOR);
    expect(LPAREN);
    firstInt = parse_assignStmnt();

    whileInt->type = CJMP;
    parse_condition(whileInt); /////////////////////////////
    expect(SEMICOLON);
    sndAssign = parse_assignStmnt();
    expect(RPAREN);
    BodyNode = parse_body();

    noop->type = NOOP;
    noop->next = nullptr;

    firstInt->next = whileInt;

    whileInt->next = BodyNode;
    whileInt->cjmp_inst.target = noop;

    temp = whileInt;

    while (temp->next != nullptr)
    {
        temp = temp->next;
    }

    temp->next = sndAssign;

    sndAssign->next = jump;

    jump->type = JMP;
    jump->jmp_inst.target = whileInt;
    jump->next = noop;

    return firstInt;
}

InstructionNode *parse_switchStmnt()
{
    expect(SWITCH);

    InstructionNode *firstInt = new InstructionNode();

    InstructionNode *noop = new InstructionNode();

    InstructionNode *temp = nullptr;

    firstInt->type = JMP;

    noop->type = NOOP;

    firstInt->next = noop;

    noop->next = nullptr;

    Token t = expect(ID);
    expect(LBRACE);
    firstInt->jmp_inst.target = parse_caseList(noop, t.lexeme);

    temp = firstInt->jmp_inst.target;

    while (temp->next != noop)
    {
        temp = temp->next;
    }

    if (lexer.peek(1).token_type == DEFAULT)
    {
        temp->next = parse_defaultCase(noop);
    }

    expect(RBRACE);

    return firstInt;
}

ConditionalOperatorType parse_relop()
{
    if (lexer.peek(1).token_type == GREATER)
    {
        expect(GREATER);
        return CONDITION_GREATER;
    }
    else if (lexer.peek(1).token_type == LESS)
    {
        expect(LESS);
        return CONDITION_LESS;
    }
    else
    {
        expect(NOTEQUAL);
        return CONDITION_NOTEQUAL;
    }
}

void parse_condition(InstructionNode *condNode)
{

    std::string oprnd1 = parse_primary();

    condNode->cjmp_inst.opernd1_index = varAddr[oprnd1];

    condNode->cjmp_inst.condition_op = parse_relop();

    std::string oprnd2 = parse_primary();

    condNode->cjmp_inst.opernd2_index = varAddr[oprnd2];
}

InstructionNode *parse_ifStmnt()
{
    InstructionNode *firstIntsr = new InstructionNode();
    InstructionNode *BodyNode = nullptr;

    firstIntsr->type = CJMP;

    expect(IF);
    parse_condition(firstIntsr);
    BodyNode = parse_body();
    firstIntsr->next = BodyNode;

    InstructionNode *noop = new InstructionNode();
    noop->type = NOOP;
    noop->next = nullptr;

    firstIntsr->cjmp_inst.target = noop;

    InstructionNode *temp = BodyNode;

    while (temp->next != nullptr)
    {
        temp = temp->next;
    }

    temp->next = noop;

    return firstIntsr;
}

InstructionNode *parse_whileStmnt()
{
    InstructionNode *firstInt = new InstructionNode();
    InstructionNode *BodyNode = nullptr;
    InstructionNode *noop = new InstructionNode();
    InstructionNode *temp = nullptr;
    InstructionNode *jump = new InstructionNode();

    firstInt->type = CJMP;

    expect(WHILE);
    parse_condition(firstInt);

    BodyNode = parse_body();

    noop->type = NOOP;
    noop->next = nullptr;

    firstInt->next = BodyNode;
    firstInt->cjmp_inst.target = noop;

    temp = firstInt;

    while (temp->next != nullptr)
    {
        temp = temp->next;
    }

    jump->type = JMP;
    jump->jmp_inst.target = firstInt;
    jump->next = nullptr;

    jump->next = noop;

    temp->next = jump;

    return firstInt;
}

InstructionNode *parse_inputStmnt()
{
    InstructionNode *firstInt = new InstructionNode();

    expect(INPUT);

    firstInt->type = IN;

    firstInt->input_inst.var_index = varAddr[expect(ID).lexeme];

    expect(SEMICOLON);

    return firstInt;
}

InstructionNode *parse_outputStmnt()
{

    InstructionNode *firstInt = new InstructionNode();

    expect(OUTPUT);

    firstInt->type = OUT;
    int i = varAddr[expect(ID).lexeme];
    firstInt->output_inst.var_index = i;

    expect(SEMICOLON);

    return firstInt;
}

ArithmeticOperatorType parse_op()
{
    if (lexer.peek(1).token_type == PLUS)
    {
        expect(PLUS);
        return OPERATOR_PLUS;
    }
    else if (lexer.peek(1).token_type == MINUS)
    {
        expect(MINUS);
        return OPERATOR_MINUS;
    }
    else if (lexer.peek(1).token_type == MULT)
    {
        expect(MULT);
        return OPERATOR_MULT;
    }
    else
    {
        expect(DIV);
        return OPERATOR_DIV;
    }
}

void parse_expr(InstructionNode *instNode)
{
    std::string oprnd1 = parse_primary();

    instNode->assign_inst.opernd1_index = varAddr[oprnd1];

    instNode->assign_inst.op = parse_op();

    std::string oprnd2 = parse_primary();

    instNode->assign_inst.opernd2_index = varAddr[oprnd2];
}

InstructionNode *parse_assignStmnt()
{

    InstructionNode *firstInt = new InstructionNode();

    firstInt->type = ASSIGN;
    std::string k = expect(ID).lexeme;
    firstInt->assign_inst.left_hand_side_index = varAddr[k]; // assigning left hand
    expect(EQUAL);

    if (lexer.peek(2).token_type == SEMICOLON)
    {
        std::string i = parse_primary();
        firstInt->assign_inst.opernd1_index = varAddr[i];
        firstInt->assign_inst.op = OPERATOR_NONE;
    }
    else
    {
        parse_expr(firstInt);
    }
    expect(SEMICOLON);

    return firstInt;
}

InstructionNode *parse_stmnt()
{
    TokenType t = lexer.peek(1).token_type;

    InstructionNode *firstInt = nullptr;

    if (t == ID)
    {
        firstInt = parse_assignStmnt();
    }
    else if (t == WHILE)
    {
        firstInt = parse_whileStmnt();
    }
    else if (t == IF)
    {
        firstInt = parse_ifStmnt();
    }
    else if (t == SWITCH)
    {
        firstInt = parse_switchStmnt();
    }
    else if (t == FOR)
    {
        firstInt = parse_forStmnt();
    }
    else if (t == INPUT)
    {
        firstInt = parse_inputStmnt();
    }
    else
    {
        firstInt = parse_outputStmnt();
    }

    return firstInt;
}

InstructionNode *parse_stmntList()
{
    bool isFOR = false;
    isFOR = (lexer.peek(1).token_type == FOR);

    bool isSwitch = false;
    isSwitch = (lexer.peek(1).token_type == SWITCH);

    InstructionNode *firstInt = parse_stmnt(); ////////there might be some error here

    InstructionNode *nodeNeededNext = firstInt;

    if (isFOR)
    {
        nodeNeededNext = firstInt->next->cjmp_inst.target;

        if (lexer.peek(1).token_type != RBRACE)
        {
            InstructionNode *tail = nullptr;
            tail = parse_stmntList();
            nodeNeededNext->next = tail;
        }

        return firstInt;
    }

    if (isSwitch)
    {
        nodeNeededNext = firstInt->next;
    }

    if (firstInt->type == CJMP)
    {
        nodeNeededNext = firstInt->cjmp_inst.target;

        if (lexer.peek(1).token_type != RBRACE)
        {
            InstructionNode *tail = nullptr;
            tail = parse_stmntList();
            nodeNeededNext->next = tail;
        }

        return firstInt;
    }

    // if (firstInt->type == JMP)
    // {
    //     nodeNeededNext = firstInt->next;
    // }

    if (lexer.peek(1).token_type != RBRACE)
    {
        InstructionNode *tail = nullptr;
        tail = parse_stmntList();
        nodeNeededNext->next = tail;
    }

    return firstInt;
}

InstructionNode *parse_body()
{
    InstructionNode *firstInt = nullptr;
    expect(LBRACE);
    firstInt = parse_stmntList();
    expect(RBRACE);
    return firstInt;
}

void parse_idList()
{
    Token t = expect(ID);
    varAddr[t.lexeme] = next_available;
    mem[next_available] = 0;
    next_available = next_available + 1;

    if (lexer.peek(1).token_type == COMMA)
    {
        expect(COMMA);
        parse_idList();
    }
}

void parse_varSection()
{
    parse_idList();
    expect(SEMICOLON);
}

void parse_program()
{
    parse_varSection();
    start = parse_body();
    parse_inputs();
};

InstructionNode *parse_Generate_Intermediate_Representation()
{
    parse_program();
    return start;
}