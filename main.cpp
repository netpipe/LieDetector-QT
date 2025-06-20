#include <QApplication>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QAudioInput>
#include <QAudioFormat>
#include <QTimer>
#include <QByteArray>
#include <QtMath>
#include <QBuffer>
#include <qaudioinput.h>
#include <vector>
#include <cmath>

#include "kiss_fft.h"

constexpr int SAMPLE_RATE = 44100;
constexpr int BUFFER_SIZE = 2048;

class LieDetectorWidget : public QWidget {
    Q_OBJECT

public:
    LieDetectorWidget(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        auto *loadButton = new QPushButton("Load WAV", this);
        resultLabel = new QLabel("Stress: -- %", this);

        layout->addWidget(loadButton);
        layout->addWidget(resultLabel);
        setLayout(layout);

        connect(loadButton, &QPushButton::clicked, this, &LieDetectorWidget::loadWav);
        startMic();
    }

private slots:
    void loadWav() {
        QString filename = QFileDialog::getOpenFileName(this, "Open WAV File", "", "*.wav");
        if (filename.isEmpty()) return;

        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) return;

        file.read(44); // skip WAV header
        QByteArray audioData = file.readAll();
        processAudio(audioData);
    }

    void onMicData() {
        QByteArray data = inputDevice->readAll(); // inputDevice should be stored as a member

        micBuffer.append(data);
        if (micBuffer.size() >= BUFFER_SIZE * 2) {
            QByteArray chunk = micBuffer.left(BUFFER_SIZE * 2);
            micBuffer.remove(0, BUFFER_SIZE * 2);
            processAudio(chunk);
        }
    }

private:
    QLabel *resultLabel;
    QAudioInput *audioInput;
    QByteArray micBuffer;
    QIODevice *inputDevice = nullptr;

    void startMic() {
        QAudioFormat format;
        format.setSampleRate(SAMPLE_RATE);
        format.setChannelCount(1);
        format.setSampleSize(16);
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
        if (!info.isFormatSupported(format)) {
            //qWarning() << "Format not supported!";
            return;
        }

        audioInput = new QAudioInput(format, this);
        inputDevice = audioInput->start();
        connect(inputDevice, &QIODevice::readyRead, this, &LieDetectorWidget::onMicData);
    }


    void processAudio(const QByteArray &data) {
        std::vector<float> samples(BUFFER_SIZE);
        const short* raw = reinterpret_cast<const short*>(data.constData());
        for (int i = 0; i < BUFFER_SIZE; ++i)
            samples[i] = raw[i] / 32768.0f;

        float stress = analyzeMicrotremor(samples);
        resultLabel->setText(QString("Stress: %1 %").arg(int(stress * 100)));
    }

    float analyzeMicrotremor(const std::vector<float>& input) {
        kiss_fft_cfg cfg = kiss_fft_alloc(BUFFER_SIZE, 0, nullptr, nullptr);
        std::vector<kiss_fft_cpx> out(BUFFER_SIZE);
        std::vector<kiss_fft_cpx> in(BUFFER_SIZE);

        for (int i = 0; i < BUFFER_SIZE; ++i) {
            in[i].r = input[i];
            in[i].i = 0;
        }

        kiss_fft(cfg, in.data(), out.data());

        float total = 0, bandEnergy = 0;
        for (int i = 1; i < BUFFER_SIZE / 2; ++i) {
            float freq = i * SAMPLE_RATE / BUFFER_SIZE;
            float mag = sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
            total += mag;
            if (freq >= 8 && freq <= 14) bandEnergy += mag;
        }

        free(cfg);
        float ratio = bandEnergy / total;
        return std::min(1.0f, ratio * 5);
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    LieDetectorWidget w;
    w.setWindowTitle("Lie Detector Demo");
    w.resize(300, 150);
    w.show();
    return app.exec();
}

#include "main.moc"
