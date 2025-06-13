import React, {createContext, useState, useContext} from 'react';
import NotificationToast from "../components/NotificationToast.jsx";

const NotificationContext = createContext(undefined);

export const NotificationProvider = ({ children }) => {
    const [notification, setNotification] = useState({
        message: '',
        type: 'success',
        isVisible: false,
    });

    const showNotification = (message, type) => {
        setNotification({ message, type, isVisible: true });
        setTimeout(() => {
            hideNotification();
        }, 3000);
    };

    const hideNotification = () => {
        setNotification(prev => ({ message: prev.message, type: prev.type, isVisible: false }));
    };

    return (
        <NotificationContext.Provider value={{ showNotification, hideNotification }}>
            {children}
            {notification.isVisible && (
                <NotificationToast
                    message={notification.message}
                    type={notification.type}
                    onClose={hideNotification}
                />
            )}
        </NotificationContext.Provider>
    );
};

export const useNotification = () => {
    const context = useContext(NotificationContext);
    if (!context) {
        throw new Error('useNotification must be used within a NotificationProvider');
    }
    return context;
};

