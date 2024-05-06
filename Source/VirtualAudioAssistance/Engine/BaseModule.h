//
// Created by Elevoc on 2021/12/14 0014.
//

#ifndef TEST1_BASEMODULE_H
#define TEST1_BASEMODULE_H

#include <string>
#include <vector>

/**
 * 子类 通过 复写 父类 的函数 达到 更改的相应 参数的目的
 */
class BaseModule {
public:

    virtual std::string getName() = 0;

    virtual int getFrameMs() { return 10; }

    virtual bool isNeedLoad() { return true; }

    virtual bool isNeedCallback() { return false; }

    virtual std::string getParam() { return mParams; }

    virtual std::vector<std::string> getDepends() { return mDepends; };

    virtual int getDependCount(){return 1;}

    virtual std::vector<std::string> getOutputs() = 0;

    BaseModule *addDepends(const std::string &depend) {
        mDepends.emplace_back(depend);
        return this;
    }
    BaseModule* setParam(const std::string &param) {
        mParams = param;
        return this;
    }

private:
    std::vector<std::string> mDepends{};
    std::string mParams;
};

class InputModule : public BaseModule {
public:
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{
                "input.mic.float.0",
                "input.ref.float.0"};
    }

    std::string getName() override {
        return "input";
    }

    int getDependCount() override{
        return 0;
    }

};

class OutputModule : public BaseModule {
public:
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"output.out.float.0"};
    }

    std::string getName() override {
        return "output";
    }
};

/// voip
class VoipAECModule :public BaseModule{
public:
    std::string getName() override{
        return "elevoc_teamsaec";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_teamsaec.output_linear.float.0","elevoc_teamsaec.output_nlp.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

class VoipDeepAecModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_deepaec";
    }
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_deepaec.out.float.0"};
    }
};

class VoipGNSModule : public BaseModule{
public:
    std::string getName() override{
        return "elevoc_gns";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_gns.out.float.0","elevoc_gns.mask.float.0"};
    }
    int getDependCount() override {
        return 2;
    }
};

// 小模型 small ns
class VoipSNSModule : public BaseModule{
public:
    std::string getName() override{
        return "elevoc_s_cns";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_s_cns.out.float.0","elevoc_s_cns.mask.float.0"};
    }
};

class VoipQNSModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_qns";
    }
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_qns.out.float.0", "elevoc_qns.mask.float.0"};
    }
    int getDependCount() override {
        return 2;
    }
};

class VoipCNSModule : public BaseModule{
public:
    std::string getName() override{
        return "elevoc_cns";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_cns.out.float.0","elevoc_cns.mask.float.0"};
    }
    int getDependCount() override {
        return 2;
    }
};

class VoipSeparationModule: public BaseModule{
public:
    std::string getName() override{
        return "elevoc_voice_separation";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_voice_separation.output.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

// post filter
class VoipPFModule: public BaseModule{
public:
    std::string getName() override{
        return "elevoc_teamstestmode";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_teamstestmode.output.float.0"};
    }
    int getDependCount() override{
        /* ori ref + aec linear + aec nonlinear + ns mask*/
        return 4;
    }
};

class KWS2Mic: public BaseModule{
public:
    std::string getName() override{
        return "elevoc_kwstwomic";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_kwstwomic.output.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

class KWS4Mic: public BaseModule{
public:
    std::string getName() override{
        return "elevoc_kwsfourmic";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_kwsfourmic.output.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

/// speech2mic
class Speech2MicModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_speechtwomic";
    }

    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_speechtwomic.output.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

class Speech4MicModule: public BaseModule{
public:
    std::string getName() override{
        return "elevoc_speechfourmic";
    }
    std::vector<std::string> getOutputs() override{
        return std::vector<std::string>{"elevoc_speechfourmic.output.float.0"};
    }
    int getDependCount() override{
        /*mic + ref*/
        return 2;
    }
};

class EQModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_teamseq";
    }
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_teamseq.output.float.0"};
    }
    int getDependCount() override {
        return 1;
    }
};


class TDNSModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_tradition_ns";
    }
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_tradition_ns.output.float.0"};
    }

    int getDependCount() override {
        return 2;
    }
};

class AGCModule : public BaseModule {
public:
    std::string getName() override {
        return "elevoc_teamsagc";
    }
    std::vector<std::string> getOutputs() override {
        return std::vector<std::string>{"elevoc_teamsagc.output.float.0"};
    }
    int getDependCount() override {
        return 1;
    }
};


#endif //TEST1_BASEMODULE_H
