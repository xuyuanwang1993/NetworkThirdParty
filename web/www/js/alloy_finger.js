/* AlloyFinger v0.1.15
 * By dntzhang
 * Github: https://github.com/AlloyTeam/AlloyFinger
 */
(function () {
    // 2点之间的距离 (主要用来算pinch的比例的, 两点之间的距离比值求pinch的scale)
    function getLen(v) {
        return Math.sqrt(v.x * v.x + v.y * v.y);
    }

    // dot和getAngle函数用来算两次手势状态之间的夹角, cross函数用来算方向的, getRotateAngle函数算手势真正的角度的
    function dot(v1, v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    // 求两次手势状态之间的夹角
    function getAngle(v1, v2) {
        var mr = getLen(v1) * getLen(v2);
        if (mr === 0) return 0;
        var r = dot(v1, v2) / mr;
        if (r > 1) r = 1;
        return Math.acos(r);
    }

    // 利用cross结果的正负来判断旋转的方向(大于0为逆时针, 小于0为顺时针)
    function cross(v1, v2) {
        return v1.x * v2.y - v2.x * v1.y;
    }

    // 如果cross大于0那就是逆时针对于屏幕是正角,对于第一象限是负角,所以 角度 * -1, 然后角度单位换算
    function getRotateAngle(v1, v2) {
        var angle = getAngle(v1, v2);
        if (cross(v1, v2) > 0) {
            angle *= -1;
        }

        return angle * 180 / Math.PI;
    }

    // HandlerAdmin构造函数
    var HandlerAdmin = function (el) {
        this.handlers = []; // 手势函数集合
        this.el = el;// dom元素
    };
    // HandlerAdmin原型方法

    // 把fn添加到实例的 handlers数组中
    HandlerAdmin.prototype.add = function (handler) {
        this.handlers.push(handler);
    }

    // 删除 handlers数组中的函数
    HandlerAdmin.prototype.del = function (handler) {
        if (!handler) this.handlers = [];// handler为假值,handlers则赋值为[](参数不传undefined,其实不管this.handlers有没有成员函数,都得置空)

        for (var i = this.handlers.length; i >= 0; i--) {
            if (this.handlers[i] === handler) {// 如果函数一样
                this.handlers.splice(i, 1);// 从handler中移除该函数(改变了原数组)
            }
        }
    }

    // 执行用户的回调函数
    HandlerAdmin.prototype.dispatch = function () {
        for (var i = 0, len = this.handlers.length; i < len; i++) {
            var handler = this.handlers[i];
            if (typeof handler === 'function') handler.apply(this.el, arguments);// 执行回调this为dom元素, 把触发的事件对象作为参数传过去了
        }
    }

    function wrapFunc(el, handler) {
        var handlerAdmin = new HandlerAdmin(el);// 实例化一个对象
        handlerAdmin.add(handler);

        return handlerAdmin;
    }

    // AlloyFinger构造函数
    var AlloyFinger = function (el, option) {// el: dom元素/id, option: 各种手势的集合对象

        this.element = typeof el == 'string' ? document.querySelector(el) : el;// 获取dom元素

        // 绑定原型上start, move, end, cancel函数的this对象为 AlloyFinger实例
        this.start = this.start.bind(this);
        this.move = this.move.bind(this);
        this.end = this.end.bind(this);
        this.cancel = this.cancel.bind(this);
        // 给dom元素 绑定原生的 touchstart, touchmove, touchend, touchcancel事件, 默认冒泡
        this.element.addEventListener("touchstart", this.start, false);
        this.element.addEventListener("touchmove", this.move, false);
        this.element.addEventListener("touchend", this.end, false);
        this.element.addEventListener("touchcancel", this.cancel, false);

        this.preV = {x: null, y: null};// 开始前的坐标
        this.pinchStartLen = null;// start()方法开始时捏的长度
        this.zoom = 1;// 初始缩放比例为1
        this.isDoubleTap = false;// 是否双击, 默认为false

        var noop = function () {
        };// 空函数(把用户没有绑定手势函数默认赋值此函数)

        // 提供了14种手势函数. 根据option对象, 分别创建一个 HandlerAdmin实例 赋值给相应的this属性
        this.rotate = wrapFunc(this.element, option.rotate || noop);
        this.touchStart = wrapFunc(this.element, option.touchStart || noop);
        this.multipointStart = wrapFunc(this.element, option.multipointStart || noop);
        this.multipointEnd = wrapFunc(this.element, option.multipointEnd || noop);
        this.pinch = wrapFunc(this.element, option.pinch || noop);
        this.swipe = wrapFunc(this.element, option.swipe || noop);
        this.tap = wrapFunc(this.element, option.tap || noop);
        this.doubleTap = wrapFunc(this.element, option.doubleTap || noop);
        this.longTap = wrapFunc(this.element, option.longTap || noop);
        this.singleTap = wrapFunc(this.element, option.singleTap || noop);
        this.pressMove = wrapFunc(this.element, option.pressMove || noop);
        this.twoFingerPressMove = wrapFunc(this.element, option.twoFingerPressMove || noop);
        this.touchMove = wrapFunc(this.element, option.touchMove || noop);
        this.touchEnd = wrapFunc(this.element, option.touchEnd || noop);
        this.touchCancel = wrapFunc(this.element, option.touchCancel || noop);

        this._cancelAllHandler = this.cancelAll.bind(this);

        window.addEventListener('scroll', this._cancelAllHandler);

        this.delta = null;// 差值 变量增量
        this.last = null; // 最后数值
        this.now = null; // 开始时的时间戳
        this.tapTimeout = null;// tap超时
        this.singleTapTimeout = null;// singleTap超时
        this.longTapTimeout = null;// longTap超时(定时器的返回值)
        this.swipeTimeout = null;// swipe超时
        this.x1 = this.x2 = this.y1 = this.y2 = null;// start()时的坐标x1, y1, move()时的坐标x2, y2 (相对于页面的坐标)
        this.preTapPosition = {x: null, y: null};// 用来保存start()方法时的手指坐标
    };

    // AlloyFinger原型对象
    AlloyFinger.prototype = {
        start: function (evt) {// 如果没有TouchList对象, 直接return掉 (touches: 位于屏幕上的所有手指的列表)
            if (!evt.touches) return;
            this.now = Date.now();// 开始时间戳
            this.x1 = evt.touches[0].pageX;// 相对于页面的 x1, y1 坐标
            this.y1 = evt.touches[0].pageY;
            this.delta = this.now - (this.last || this.now);// 时间戳差值

            this.touchStart.dispatch(evt, this.element);// 调用HandlerAdmin实例this.touchStart上的dispatch方法(用户的touchStart回调就在此调用的)

            if (this.preTapPosition.x !== null) { // 开始前tap的x坐标不为空的话(一次没点的时候必然是null了)
                this.isDoubleTap = (this.delta > 0 && this.delta <= 250 && Math.abs(this.preTapPosition.x - this.x1) < 30 && Math.abs(this.preTapPosition.y - this.y1) < 30);
                if (this.isDoubleTap) clearTimeout(this.singleTapTimeout);
            }
            this.preTapPosition.x = this.x1;// 把相对于页面的 x1, y1 坐标赋值给 this.preTapPosition
            this.preTapPosition.y = this.y1;
            this.last = this.now;// 把开始时间戳赋给 this.last
            var preV = this.preV,// 把开始前的坐标赋给 preV变量
                len = evt.touches.length;// len: 手指的个数
            if (len > 1) {// 一根手指以上
                this._cancelLongTap();// 取消长按定时器
                this._cancelSingleTap();// 取消SingleTap定时器

                var v = { // 2个手指坐标的差值
                    x: evt.touches[1].pageX - this.x1,
                    y: evt.touches[1].pageY - this.y1
                };
                preV.x = v.x;// 差值赋值给PreV对象
                preV.y = v.y;
                this.pinchStartLen = getLen(preV); // start()方法中2点之间的距离
                this.multipointStart.dispatch(evt, this.element);// (用户的multipointStart（缩放）回调就在此调用的)
                //alert(preV.x+":::"+preV.y+":::"+this.element+":::"+len);
            }
            this._preventTap = false;
            this.longTapTimeout = setTimeout(function () {
                this.longTap.dispatch(evt, this.element);// (用户的longTap（长按）回调就在此调用的)
                this._preventTap = true;
            }.bind(this), 750);
        },
        move: function (evt) {
            if (!evt.touches) return; // 如果没有TouchList对象, 直接return掉 (touches: 位于屏幕上的所有手指的列表)

            var preV = this.preV,// 把start方法保存的2根手指坐标的差值xy赋给preV变量
                len = evt.touches.length,// 手指个数
                currentX = evt.touches[0].pageX,// 第一根手指的坐标(相对于页面的 x1, y1 坐标)
                currentY = evt.touches[0].pageY;
            this.isDoubleTap = false;// 移动过程中不能双击了

            if (len > 1) {// 2根手指以上(处理捏pinch和旋转rotate手势)
                var sCurrentX = evt.touches[1].pageX,
                    sCurrentY = evt.touches[1].pageY
                var v = { // 第二根手指和第一根手指坐标的差值
                    x: evt.touches[1].pageX - currentX,
                    y: evt.touches[1].pageY - currentY
                };

                if (preV.x !== null) { // start方法中保存的this.preV的x不为空的话
                    if (this.pinchStartLen > 0) {// 2点间的距离大于0
                        evt.zoom = getLen(v) / this.pinchStartLen;// move中的2点之间的距离除以start中的2点的距离就是缩放比值
                        this.pinch.dispatch(evt, this.element);// scale挂在到evt对象上 (用户的pinch回调就在此调用的)
                        //alert(this.pinchStartLen+":::"+getLen(v)+":::"+this.element+":::"+v);
                    }
                    evt.angle = getRotateAngle(v, preV);// 计算angle角度
                    this.rotate.dispatch(evt, this.element); // (用户的pinch回调就在此调用的)
                }
                preV.x = v.x;// 把move中的2根手指中的差值赋值给preV, 当然也改变了this.preV
                preV.y = v.y;

                if (this.x2 !== null && this.sx2 !== null) {
                    evt.deltaX = (currentX - this.x2 + sCurrentX - this.sx2) / 2;
                    evt.deltaY = (currentY - this.y2 + sCurrentY - this.sy2) / 2;
                } else {
                    evt.deltaX = 0;
                    evt.deltaY = 0;
                }
                this.twoFingerPressMove.dispatch(evt, this.element);

                this.sx2 = sCurrentX;
                this.sy2 = sCurrentY;
            } else {// 一根手指(处理拖动pressMove手势)
                if (this.x2 !== null) {
                    evt.deltaX = currentX - this.x2;// 拖动过程中或者手指移动过程中的差值(当前坐标与上一次的坐标)
                    evt.deltaY = currentY - this.y2;

                    //move事件中添加对当前触摸点到初始触摸点的判断，
                    //如果曾经大于过某个距离(比如10),就认为是移动到某个地方又移回来，应该不再触发tap事件才对。
                    var movedX = Math.abs(this.x1 - this.x2),
                        movedY = Math.abs(this.y1 - this.y2);

                    if (movedX > 10 || movedY > 10) {
                        this._preventTap = true;
                    }

                } else { // 第一次嘛, 手指刚移动,哪来的差值啊,所以为0呗
                    evt.deltaX = 0;
                    evt.deltaY = 0;
                }

                this.pressMove.dispatch(evt, this.element);// deltaXY挂载到evt对象上,抛给用户(用户的pressMove回调就在此调用的)
            }

            this.touchMove.dispatch(evt, this.element); // evt对象因if语句而不同,挂载不同的属性抛出去给用户 (用户的touchMove回调就在此调用的)

            this._cancelLongTap();// 取消长按定时器
            this.x2 = currentX;// 存一下本次的pageXY坐标, 为了下次做差值
            this.y2 = currentY;

            if (len > 1) {// 2个手指以上就阻止默认事件
                evt.preventDefault();
            }
        },
        end: function (evt) {
            if (!evt.changedTouches) return;// 位于该元素上的所有手指的列表, 没有TouchList也直接return掉
            this._cancelLongTap();// 取消长按定时器
            var self = this;// 存个实例
            if (evt.touches.length < 2) {// 手指数量小于2就触发 (用户的多点结束multipointEnd回调函数)
                this.multipointEnd.dispatch(evt, this.element);
                this.sx2 = this.sy2 = null;
            }

            ////swipe 滑动
            if ((this.x2 && Math.abs(this.x1 - this.x2) > 30) ||
                (this.y2 && Math.abs(this.y1 - this.y2) > 30)) {
                evt.direction = this._swipeDirection(this.x1, this.x2, this.y1, this.y2);// 获取上下左右方向中的一个
                this.swipeTimeout = setTimeout(function () {
                    self.swipe.dispatch(evt, self.element);// 立即触发,加入异步队列(用户的swipe事件的回调函数)
                }, 0)
            } else {// 以下是tap, singleTap, doubleTap事件派遣
                this.tapTimeout = setTimeout(function () {
                    if (!self._preventTap) {
                        self.tap.dispatch(evt, self.element);// 触发(用户的tap事件的回调函数)
                    }
                    // trigger double tap immediately
                    if (self.isDoubleTap) {// 如果满足双击的话
                        self.doubleTap.dispatch(evt, self.element);// 触发(用户的doubleTap事件的回调函数)双击事件
                        clearTimeout(self.singleTapTimeout);    // 清除singleTapTimeout定时器
                        self.isDoubleTap = false; // 双击条件重置

                    }
                }, 0)// 加入异步队列,主线程完成立马执行

                if (!self.isDoubleTap) {
                    self.singleTapTimeout = setTimeout(function () {
                        self.singleTap.dispatch(evt, self.element);// 触发(用户的singleTap事件的回调函数)
                    }, 250);
                }
            }

            this.touchEnd.dispatch(evt, this.element);// 触发(用户的touchEnd回调函数)

            this.preV.x = 0;// this.preV, this.scale, this.pinchStartLen, this.x1 x2 y1 y2恢复初始值
            this.preV.y = 0;
            this.zoom = 1;
            this.pinchStartLen = null;
            this.x1 = this.x2 = this.y1 = this.y2 = null;
        },
        cancelAll: function () {
            //清除定时器
            // 关闭所有定时器
            this._preventTap = true
            clearTimeout(this.singleTapTimeout);
            clearTimeout(this.tapTimeout);
            clearTimeout(this.longTapTimeout);
            clearTimeout(this.swipeTimeout);
        },
        cancel: function (evt) {
            this.cancelAll()
            this.touchCancel.dispatch(evt, this.element);// 执行用户的touchCancel回调函数,没有就执行一次noop空函数
        },
        _cancelLongTap: function () { // 取消长按定时器
            clearTimeout(this.longTapTimeout);
        },
        _cancelSingleTap: function () {// 取消延时SingleTap定时器
            clearTimeout(this.singleTapTimeout);
        },
        // 2点间x与y之间的绝对值的差值作比较,x大的话即为左右滑动,y大即为上下滑动,x的差值大于0即为左滑动,小于0即为右滑动
        _swipeDirection: function (x1, x2, y1, y2) { // 判断用户到底是从上到下，还是从下到上，或者从左到右、从右到左滑动
            return Math.abs(x1 - x2) >= Math.abs(y1 - y2) ? (x1 - x2 > 0 ? 'Left' : 'Right') : (y1 - y2 > 0 ? 'Up' : 'Down')
        },

        // 给dom添加14种事件中的一种
        on: function (evt, handler) {
            alert(this[evt]+":::"+handler);
            if (this[evt]) { // 事件名在这14中之中，才添加函数到监听事件中
                this[evt].add(handler);// HandlerAdmin实例的add方法
            }

        },

        // 移除手势事件对应函数
        off: function (evt, handler) {
            alert(this[evt]+":::"+handler);
            if (this[evt]) {// 事件名在这14中之中，才移除相应监听函数
                this[evt].del(handler);// 从数组中删除handler方法
            }

        },

        // 清空，重置所有数据
        destroy: function () {
            // 关闭所有定时器
            if (this.singleTapTimeout) clearTimeout(this.singleTapTimeout);
            if (this.tapTimeout) clearTimeout(this.tapTimeout);
            if (this.longTapTimeout) clearTimeout(this.longTapTimeout);
            if (this.swipeTimeout) clearTimeout(this.swipeTimeout);

            // 取消dom上touchstart, touchmove, touchend, touchcancel事件
            // 移除touch的四个事件
            this.element.removeEventListener("touchstart", this.start);
            this.element.removeEventListener("touchmove", this.move);
            this.element.removeEventListener("touchend", this.end);
            this.element.removeEventListener("touchcancel", this.cancel);

            // 把14个HandlerAdmin实例的this.handlers置为空数组
            // 清除所有手势的监听函数
            this.rotate.del();
            this.touchStart.del();
            this.multipointStart.del();
            this.multipointEnd.del();
            this.pinch.del();
            this.swipe.del();
            this.tap.del();
            this.doubleTap.del();
            this.longTap.del();
            this.singleTap.del();
            this.pressMove.del();
            this.twoFingerPressMove.del();
            this.touchMove.del();
            this.touchEnd.del();
            this.touchCancel.del();

            // 重置所有变量
            this.preV = this.pinchStartLen = this.zoom = this.isDoubleTap = this.delta = this.last = this.now = this.tapTimeout = this.singleTapTimeout = this.longTapTimeout = this.swipeTimeout = this.x1 = this.x2 = this.y1 = this.y2 = this.preTapPosition = this.rotate = this.touchStart = this.multipointStart = this.multipointEnd = this.pinch = this.swipe = this.tap = this.doubleTap = this.longTap = this.singleTap = this.pressMove = this.touchMove = this.touchEnd = this.touchCancel = this.twoFingerPressMove = null;

            window.removeEventListener('scroll', this._cancelAllHandler);
            return null;
        }
    };

    // 如果当前环境支持module，exports等es6语法，则导出AlloyFingerPlugin模块
    if (typeof module !== 'undefined' && typeof exports === 'object') {
        module.exports = AlloyFinger;
    } else {// 否则将AlloyFingerPlugin注册到全局对象
        window.AlloyFinger = AlloyFinger;
    }
})();
