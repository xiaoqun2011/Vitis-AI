#include <utility>
#include <map>
#include <vector>
#include <string>

#include "caffe/proto/caffe.pb.h"
#include "caffe/common.hpp"
#include "caffe/util/io.hpp"
#include "caffe/util/upgrade_proto.hpp"

DEFINE_bool(remove_blobs, true, "Remove the blobs in the caffemodel");
DEFINE_bool(
    remove_split, true,
    "Remove the split layers auto generated by caffe in the caffemodel");

using namespace caffe;
using namespace std;

NetParameter RemoveSplit(const NetParameter& model_in) {
    NetParameter model_out;
    map<string, string> split_blobs;
    for (size_t i = 0; i < model_in.layer_size(); i++) {
        LOG(INFO) << "layer: " << model_in.layer(i).name();
        if (model_in.layer(i).type() == string("Split")) {
            CHECK_EQ(model_in.layer(i).bottom_size(), 1);
            for (size_t j = 0; j < model_in.layer(i).top_size(); j++) {
                split_blobs.insert(make_pair(model_in.layer(i).top(j),
                                             model_in.layer(i).bottom(0)));
            }
        } else {
            LayerParameter* layer = model_out.add_layer();
            *layer = model_in.layer(i);
            for (size_t i = 0; i < layer->bottom_size(); i++) {
                auto res = split_blobs.find(layer->bottom(i));
                if (res != split_blobs.end()) {
                    layer->set_bottom(i, res->second);
                }
            }
        }
    }
    return model_out;
}

NetParameter RemoveBlobs(const NetParameter& model_in) {
    NetParameter model_out = model_in;
    for (size_t i = 0; i < model_out.layer_size(); i++) {
        model_out.mutable_layer(i)->clear_blobs();
    }
    return model_out;
}

int main(int argc, char** argv) {
    FLAGS_alsologtostderr = 1;
    ::google::InitGoogleLogging(argv[0]);
    ::google::SetUsageMessage(
        "\nusage: binary_to_text [OPTION] binary_model_in text_model_out");
    ::google::ParseCommandLineFlags(&argc, &argv, true);

    if (argc == 3) {
        NetParameter model;
        ReadNetParamsFromBinaryFileOrDie(argv[1], &model);
        if (FLAGS_remove_blobs) {
            model = RemoveBlobs(model);
        }
        if (FLAGS_remove_split) {
            model = RemoveSplit(model);
        }

        WriteProtoToTextFile(model, argv[2]);

        return 0;
    } else {
        ::google::ShowUsageWithFlagsRestrict(argv[0], "tools/binary_to_text");
        return -1;
    }
}