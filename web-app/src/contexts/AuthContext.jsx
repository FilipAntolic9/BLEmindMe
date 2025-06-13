import {createContext, useContext, useEffect, useState} from 'react';
import {createCustomer, createUser, getCurrentUserInfo, getUserActivationLink, login} from '../services/AuthService';

const AuthContext = createContext(undefined);

// for local development purposes only
const TENANT_USERNAME = "luka.ivekovic@fer.hr";
const TENANT_PASSWORD = "Intstv123";

export const AuthProvider = ({children}) => {
    const [user, setUser] = useState(null);

    useEffect(() => {
        const storedUser = localStorage.getItem('user');
        if (storedUser) {
            setUser(JSON.parse(storedUser));
        }
    }, []);

    const loginUser = async (email, password) => {
        // eslint-disable-next-line no-useless-catch
        try {
            const response = await login(email, password);
            const tokenData =
            {
                accessToken: response.token ?? ""
            };

            setUser(tokenData); // so that getCurrentUserInfo has a token inside request
            localStorage.setItem('user', JSON.stringify(tokenData));

            const userInfo = await getCurrentUserInfo();
            const userData =
            {
                accessToken: response.token ?? "",
                email: email,
                role: userInfo?.authority ?? "CUSTOMER_USER",
                customerId: userInfo?.customerId?.id ?? "",
                tenantId: userInfo?.tenantId?.id ?? "",
            };
            setUser(userData);
            localStorage.setItem('user', JSON.stringify(userData));

        } catch (error) {
            throw error;
        }
    };

    const logoutUser = () => {
        setUser(null);
        localStorage.removeItem('user');
    };

    const registerUser = async (userData) => {
        // eslint-disable-next-line no-useless-catch
        try {
            await loginUser(TENANT_USERNAME, TENANT_PASSWORD);

            const createdCustomer = await createCustomer(userData.firstName, userData.email, userData.phoneNumber);
            const createdUser = await createUser(userData.email, userData.firstName, userData.lastName, userData.phoneNumber, createdCustomer.id.id);

            const activationLink = await getUserActivationLink(createdUser.id.id);

            await logoutUser();

            return activationLink;

        } catch (error) {
            throw error;
        }
    };

    return (
        <AuthContext.Provider
            value={{user, login: loginUser, logout: logoutUser, register: registerUser}}>
            {children}
        </AuthContext.Provider>
    );
};

export const useAuth = () => {
    const context = useContext(AuthContext);
    if (context === undefined) {
        throw new Error('useAuth must be used within an AuthProvider');
    }
    return context;
};