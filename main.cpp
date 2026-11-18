#include <QApplication>
#include <QElapsedTimer>
#include <QFont>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QWidget>

class clockThing : public QWidget {
public:
    enum Mode {StopwatchMode, TimerMode};

    clockThing() {
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        setWindowOpacity(0.9);
        setStyleSheet("background-color: #111111;");
        setMinimumSize(160, 60);

        label = new QLabel(this);
        QFont font("DejaVu Sans Mono");
        font.setBold(true);
        label->setFont(font);
        label->setStyleSheet(normalStyle);
        label->setAlignment(Qt::AlignCenter);
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *layout = new QVBoxLayout(this);

        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(label);

        connect(&timer, &QTimer::timeout, this, [this] { refresh(); });
        timer.start(50);
        updateFont();
        applyMode();
    }

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) {
            dragging = true;
            moved = false;
            dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        else if (e->button() == Qt::RightButton) {
            reset();
        } else if (e->button() == Qt::MiddleButton) {
            mode = (mode == StopwatchMode) ? TimerMode : StopwatchMode;
            reset();
            applyMode();
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (dragging) {
            moved = true;
            move(e->globalPosition().toPoint() - dragOffset);
        }
    }

    void mouseReleaseEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton && dragging) {
            dragging = false;
            if (!moved) toggle();
        }
    }

    void wheelEvent(QWheelEvent *e) override {
        if (mode != TimerMode || running) return;
        qint64 steps = e->angleDelta().y() / 120; // e is the QWheelEvent that Qt passes in, angleDelta() returns how far the wheel moved
        // .y() is the vertical part, scroll up = (+), scroll down = (-)
        // Qt measures the wheel in eighths of a degree, standard scrolling is 15 degrees, which is 120 units
        // dividing by 120 converts raw movement into whole notches
        duration = qBound<qint64>(0, duration + steps * 60000, 24LL * 3600 * 1000);
        // qint is signed integer for Qt
        refresh();
    }

    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        updateFont();
    }

private:
    qint64 currentMs() const {
        return banked + (running ? clock.elapsed() : 0);
    }

    bool finished() const {
        return mode == TimerMode && duration > 0 && currentMs() >= duration;
    }

    void toggle() {
        if (running) {
            banked += clock.elapsed();
            running = false;
        } else {
            if (mode == TimerMode && duration == 0) return;
            if (finished()) banked = 0; // restarts a timer
            clock.start();
            running = true;
        }
        refresh();
    }

    void reset() {
        running = false;
        banked = 0;
        refresh();
    }

    void applyMode() {
        setWindowTitle(mode == TimerMode ? "Timer" : "Stopwatch");
        refresh();
    }

    void refresh() {
        if (running && finished()) {
            banked = duration;
            running = false;
            QApplication::beep();
        }

        bool done = finished();
        if (done != showingDone) {
            showingDone = done;
            label->setStyleSheet(done ? doneStyle : normalStyle);
        }

        qint64 ms = (mode == TimerMode) ? qMax<qint64>(0, duration - currentMs()) : currentMs();
        label->setText(format(ms));
    }

    static QString format(qint64 ms) {
        qint64 cs = ms / 10;
        qint64 h = cs / 360000;
        qint64 m = (cs / 6000) % 60;
        qint64 s = (cs / 100) % 60;
        qint64 c = cs % 100;

        auto two = [](qint64 v) { return QString::number(v).rightJustified(2, '0'); };
        return h > 0
            ? QString("%1:%2:%3.%4").arg(h).arg(two(m)).arg(two(s)).arg(two(c))
            : QString("%1:%2.%3").arg(two(m)).arg(two(s)).arg(two(c));
    }

    void updateFont() {
        QFont f = label->font();
        int size = qMax(8, qMin(height() * 6 / 10, width() / 6));
        f.setPixelSize(size);
        label->setFont(f);
    }

    QLabel *label = nullptr;
    QTimer timer;
    QElapsedTimer clock;
    Mode mode = StopwatchMode;

    qint64 banked = 0; // ms from earlier runs
    qint64 duration = 10 * 60 * 1000; // timer length, defaults to 10 mins

    bool running = false;
    bool showingDone = false;
    bool dragging = false;
    bool moved = false;

    QPoint dragOffset;

    const QString normalStyle = "color: #e6e6e6;";
    const QString doneStyle = "color: #ff5555;";
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    clockThing w;
    w.show();
    return app.exec();
}