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
#include <QKeyEvent>

class clockThing : public QWidget {
public:
    enum Mode {StopwatchMode, TimerMode};

    struct ModeState {
        bool running = false;
        qint64 banked = 0;
        QElapsedTimer clock;
    };

    clockThing() {
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        setWindowOpacity(0.9);

        setFocusPolicy(Qt::StrongFocus);
        setFocus();

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
        if (mode != TimerMode || states[TimerMode].running) return;
        accumulated += e->angleDelta().y(); //Add this event's movement to the total
        qint64 steps = accumulated / 120; // whole notches accumulated so far
        accumulated -= steps*120;
        duration = qBound<qint64>(0, duration + steps * 60000, 24LL * 3600 * 1000);
        refresh();
    }

    void resizeEvent(QResizeEvent *e) override {
        QWidget::resizeEvent(e);
        updateFont();
    }

    void keyPressEvent(QKeyEvent *e) override {
        if(e->key() == Qt::Key_R && !e->isAutoRepeat()) {
            mode = (mode == StopwatchMode) ? TimerMode : StopwatchMode;
            applyMode();
        } else {
            QWidget::keyPressEvent(e);
        }
    }

private:
    
    qint64 elapsedMs(const ModeState &s) const {
        return s.banked + (s.running ? s.clock.elapsed() : 0);
    }

    bool isDone (Mode m) const {
        return m == TimerMode && duration > 0 && elapsedMs(states[m]) >= duration;
    }

    void toggle() {
        ModeState &s = states[mode];
        if (s.running) {
            s.banked += s.clock.elapsed();
            s.running = false;
        } else {
            if (mode == TimerMode && duration == 0) return;
            if (isDone(mode)) s.banked = 0; // restarts a timer
            s.clock.start();
            s.running = true;
        }
        refresh();
    }

    void reset() {
        states[mode].running = false;
        states[mode].banked = 0;
        refresh();
    }

    void applyMode() {
        setWindowTitle(mode == TimerMode ? "Timer" : "Stopwatch");
        refresh();
    }

    void refresh() {
        ModeState &t = states[TimerMode];
        if (t.running && isDone(TimerMode)) {
            t.banked = duration;
            t.running = false;
            QApplication::beep();
        }

        bool done = isDone(mode);
        if (done != showingDone) {
            showingDone = done;
            label->setStyleSheet(done ? doneStyle : normalStyle);
        }

        qint64 ms = (mode == TimerMode) ? qMax<qint64>(0, duration - elapsedMs(states[mode])) : elapsedMs(states[mode]);
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
    Mode mode = StopwatchMode;
    int accumulated = 0; // accumulated wheel movement, in eighths of a degree

    qint64 duration = 10 * 60 * 1000; // timer length, defaults to 10 mins

    bool showingDone = false;
    bool dragging = false;
    bool moved = false;

    QPoint dragOffset;

    ModeState states[2]; // 0 = stopwatch, 1 = timer

    const QString normalStyle = "color: #e6e6e6;";
    const QString doneStyle = "color: #ff5555;";
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    clockThing w;
    w.show();
    return app.exec();
}