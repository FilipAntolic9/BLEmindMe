export const requestNotificationPermission = async () => {
    if (!('Notification' in window)) {
        throw new Error('This browser does not support notifications');
    }

    const permission = await Notification.requestPermission();
    if (permission !== 'granted') {
        throw new Error('Notification permission denied');
    }

    return permission;
};
